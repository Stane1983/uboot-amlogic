// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <asm/amlogic/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <asm/amlogic/arch/mailbox.h>
#include <asm/amlogic/arch/secure_apb.h>

#define aml_writel32(value, reg)	writel(value, reg)
#define aml_readl32(reg)		readl(reg)

static inline void mbwrite(u32 to, void *from, long count)
{
	int i = 0;
	int len = count / 4 + ((count % 4) ? 1 : 0);
	u32 *p = from;

	while (len > 0) {
		aml_writel32(p[i], to + (4 * i));
		len--;
		i++;
	}
}

static inline void mbread(void *to, u32 from, long count)
{
	int i = 0;
	int len = count / 4 + ((count % 4) ? 1 : 0);
	u32 *p = to;

	while (len > 0) {
		p[i] = aml_readl32(from + (4 * i));
		len--;
		i++;
	}
}

static inline void mbclean(u32 to, long count)
{
	int i = 0;
	int len = count / 4 + ((count % 4) ? 1 : 0);

	while (len > 0) {
		aml_writel32(0, to + (4 * i));
		len--;
		i++;
	}
}

/* Return if AOCPU is alive by checking AOCPU tick count */
static u32 is_aocpu_alive(void)
{
	u32 aocpu_tick_count_1;
	u32 aocpu_tick_count_2;
	u32 ret;

	aocpu_tick_count_1 = aml_readl32(AOCPU_TICK_CNT_RD_ADDR);
	printf("%s: aocpu_tick_count_1 = %x\n", __func__, aocpu_tick_count_1);
	/* Wait for 50 msec to let tick count update */
	mdelay(50);
	aocpu_tick_count_2 = aml_readl32(AOCPU_TICK_CNT_RD_ADDR);
	printf("%s: aocpu_tick_count_2 = %x\n", __func__, aocpu_tick_count_2);
	if (aocpu_tick_count_1 == aocpu_tick_count_2)
		ret = 0;
	else
		ret = 1;

	return ret;
}

/* Return timerE value (seconds) */
static u32 read_timer_e(void)
{
	unsigned long long time64 = 0, time_high = 0;
	u32 time = 0;

	/*timeE high+low, first read low, second read high*/
	time64 = aml_readl32(TIMERE_LOW_REG);
	time_high = aml_readl32(TIMERE_HIG_REG);
	time64 += (time_high << 32);
	time64 = time64 / 1000000;
	time = (u32)time64;

	return time;
}

int mhu_get_addr(u32 chan, u32 *mboxset, u32 *mboxsts,
		 u32 *mboxwr, u32 *mboxrd,
		 u32 *mboxirqclr, u32 *mboxid)
{
	int ret = 0;

	switch (chan) {
	case AOCPU_REE_CHANNEL:
		*mboxset = REE2AO_SET_ADDR;
		*mboxsts = REE2AO_STS_ADDR;
		*mboxwr = REE2AO_WR_ADDR;
		*mboxrd = REE2AO_RD_ADDR;
		*mboxirqclr = REE2AO_IRQCLR_ADDR;
		*mboxid = REE2AO_MBOX_ID;
		break;
	default:
		printf("[BL33]: no support chan 0x%x\n", chan);
		ret = -1;
		break;
	};
	return ret;
}

void mhu_message_start(u32 mboxsts)
{
	/* Make sure any previous command has finished */
	while (aml_readl32(mboxsts) != 0)
		;
}

void mhu_build_payload(u32 mboxwr, void *message, u32 size)
{
	if (size > (MHU_PAYLOAD_SIZE - MHU_DATA_OFFSET)) {
		printf("bl33: scpi send input size error\n");
		return;
	}
	if (size == 0)
		return;
	mbwrite(mboxwr + MHU_DATA_OFFSET, message, size);
}

void mhu_get_payload(u32 mboxwr, u32 mboxrd, void *message, u32 size)
{
	if (size > (MHU_PAYLOAD_SIZE - MHU_DATA_OFFSET)) {
		printf("bl33: scpi send input size error\n");
		return;
	}
	if (size == 0)
		return;
	mbread(message, mboxrd + MHU_DATA_OFFSET, size);
	mbclean(mboxwr, MHU_PAYLOAD_SIZE);
}

void mhu_message_send(u32 mboxset, u32 command, u32 size)
{
	u32 mbox_cmd;

	mbox_cmd = MHU_CMD_BUILD(command, size + MHU_DATA_OFFSET);
	aml_writel32(mbox_cmd, mboxset);
}

u32 mhu_message_wait(u32 mboxset, u32 mboxsts, u32 command, u32 size)
{
	/* Wait for response from other core */
	u32 response;
	u32 time_stamp_start = 0;
	u32 time_stamp_end = 0;
	u32 time_elapse = 0;
	u32 retry_cnt = 2;
	u32 aocpu_alive;
	u32 mbox_cmd;

	time_stamp_start = read_timer_e();
	while ((response = aml_readl32(mboxsts))) {
		time_stamp_end = read_timer_e();
		time_elapse = time_stamp_end - time_stamp_start;
		/* Set mailbox wait time out: 1 second */
		if (time_elapse > 1) {
			printf("%s: warning: mailbox wait time out, send fail!\n", __func__);
			aocpu_alive = is_aocpu_alive();
			if (aocpu_alive)
				printf("%s: aocpu is alive!\n", __func__);
			else
				printf("%s: aocpu is not alive!\n", __func__);

			if (retry_cnt == 0)
				break;
			retry_cnt--;

			printf("%s: mailbox send retry\n", __func__);
			mbox_cmd = MHU_CMD_BUILD(command, size + MHU_DATA_OFFSET);
			aml_writel32(0, mboxset);
			aml_writel32(mbox_cmd, mboxset);
			time_stamp_start = read_timer_e();
		}
	}

	return response;
}

void mhu_message_end(u32 mboxwr, u32 mboxirqclr, u32 mboxid)
{
	/* Clear all datas in mailbox buffer */
	mbclean(mboxwr, MHU_PAYLOAD_SIZE);
	/* Clear irq_clr */
	aml_writel32(MHU_ACK_MASK(mboxid), mboxirqclr);
}

void mhu_init(void)
{
	aml_writel32(0xffffffffu, REE2AO_CLR_ADDR);
	printf("[BL33] mhu init done fifo-v2\n");
}

int scpi_send_data(u32 chan, u32 command,
		   void *sendmessage, u32 sendsize,
		   void *revmessage, u32 revsize)
{
	u32 mboxset = 0;
	u32 mboxsts = 0;
	u32 mboxwr = 0;
	u32 mboxrd = 0;
	u32 mboxirqclr = 0;
	u32 mboxid = 0;
	int ret = 0;

	ret = mhu_get_addr(chan, &mboxset, &mboxsts,
			   &mboxwr, &mboxrd,
			   &mboxirqclr, &mboxid);
	if (ret) {
		printf("bl33: mhu get addr fail\n");
		return ret;
	}
	mhu_message_start(mboxsts);
	if (sendmessage && sendsize != 0)
		mhu_build_payload(mboxwr, sendmessage, sendsize);
	mhu_message_send(mboxset, command, sendsize);
	mhu_message_wait(mboxset, mboxsts, command, sendsize);
	if (revmessage && revsize != 0)
		mhu_get_payload(mboxwr, mboxrd, revmessage, revsize);
	mhu_message_end(mboxwr, mboxirqclr, mboxid);
	return ret;
}
