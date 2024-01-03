// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <errno.h>
#include <cli.h>
#include <exports.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <abuf.h>

#ifdef CONFIG_MISC_INIT_R
#define _AML_MISC_INTERRUPT_KEY 0x09
static int _aml_interrupt_key_pressed;
static int ctrli(void)
{
	if (1/*gd->have_console*/) {
		if (tstc()) {
			switch (getchar()) {
			case _AML_MISC_INTERRUPT_KEY:/* ^I - Control I */
				_aml_interrupt_key_pressed = 1;
				printf("Detect Ctrl I...\t\nInput yes to force stopped anyway:\t");
				return 1;
			default:
				break;
			}
		}
	}

	return 0;
}

static int aml_misc_confirm_yesno(const int tm/*timeout in ms*/)
{
	int i;
	unsigned long ts;
	char str_input[5];

	_aml_interrupt_key_pressed = 0;
	if (!ctrli()) {//not detect ctrl-I when bootup
		return 0;
	}
	ts = get_timer(0);

	for (i = 0; i < sizeof(str_input);) {
		int c = 0;

		while (!tstc()) {
			if (get_timer(ts) >= tm) {
				printf("Input timeout\n");
				return 0;
			}
		}
		c = getchar();
		if (i == 0 && c == _AML_MISC_INTERRUPT_KEY) {//drop first duplicated ctrlI
			printf("Wait YES/y input\n");
			continue;
		}
		putc(c);
		str_input[i++] = c;
		if (c == '\r')
			break;
	}
	putc('\n');
	if (strncmp(str_input, "y\r", 2) == 0 ||
	    strncmp(str_input, "Y\r", 2) == 0 ||
	    strncmp(str_input, "yes\r", 4) == 0 ||
	    strncmp(str_input, "YES\r", 4) == 0)
		return 1;
	return 0;
}

int misc_init_r(void)
{
	printf("board common misc_init\n");
	if (!aml_misc_confirm_yesno(5000))
		return 0;

	cli_init();
	cli_loop();
	panic("No CLI available");
	return 0;
}
#endif // #ifdef CONFIG_MISC_INIT_R

#ifdef CONFIG_BOARD_RNG_SEED
unsigned int random(void)
{
	volatile unsigned int val;

#ifdef CONFIG_HW_RNG_OLD
	val = readl(RNG_USR_DATA);
#else
	do {} while (readl(RNG_REE_READY) & 0x1);
	do {} while (readl(RNG_REE_CFG) & 0x1);
	writel(readl(RNG_REE_CFG) | (1 << 31), RNG_REE_CFG);
	do {} while (readl(RNG_REE_CFG) >> 31);

	val = readl(RNG_REE_OUT0);
	writel(0x1, RNG_REE_READY);
#endif
	return val;
}

void get_rng_hw(unsigned char *buf, unsigned int size)
{
	unsigned int *p = (unsigned int *)buf;
	unsigned int cnt, ncnt;
	unsigned int i;
	unsigned int rng;

	cnt = (size >> 2);
	ncnt = size & 0x3;

	for (i = 0; i < cnt; i++)
		p[i] = random();

	if (ncnt) {
		rng = random();
		for (i = 0; i < ncnt; i++)
			buf[cnt * 4 + i] = ((rng >> (i * 8)) & 0xff);
	}
}

int board_rng_seed(struct abuf *buf)
{
	abuf_init(buf);
	abuf_realloc(buf, 128);
	if (buf->data)
		get_rng_hw(buf->data, buf->size);

	return 0;
}
#endif

#ifdef CONFIG_AML_DEFENV
const char * const _aml_env_reserv_array[] = {
	"lock",
	"upgrade_step",
	"bootloader_version",
	"hdmimode",
	"outputmode",
	"dts_to_gpt",
	"fastboot_step",
	"reboot_status",
	"expect_index",
	"defenv_para",	//set in board_late_init
	NULL//Keep NULL be last to tell END
};
#endif//#ifdef CONFIG_AML_DEFENV

