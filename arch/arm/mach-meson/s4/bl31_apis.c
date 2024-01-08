// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * Trustzone API
 */

#include <asm/arch/io.h>
#include <asm/arch/efuse.h>
#include <asm/cache.h>
#include <asm/arch/bl31_apis.h>
#include <amlogic/cpu_id.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/cpu_config.h>
#include <linux/arm-smccc.h>

static long sharemem_input_base;
static long sharemem_output_base;

long get_sharemem_info(unsigned long function_id)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

int32_t set_boot_params(const keymaster_boot_params *boot_params)
{
	struct arm_smccc_res res;

	if (!boot_params)
		return -1;

	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);

	memcpy((void *)sharemem_input_base,
			(const void *)boot_params, sizeof(keymaster_boot_params));

	arm_smccc_smc(SET_BOOT_PARAMS, sizeof(keymaster_boot_params), 0, 0, 0, 0, 0, 0, &res);

	return (!res.a0) ? -1 : 0;
}

#ifdef CONFIG_EFUSE
#ifdef CONFIG_EFUSE_OBJ_API
uint32_t meson_efuse_obj_read(uint32_t obj_id, uint8_t *buff, uint32_t *size)
{
	uint32_t rc = EFUSE_OBJ_ERR_UNKNOWN;
	uint32_t len = 0;
	struct arm_smccc_res res;

	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	memcpy((void *)sharemem_input_base, buff, *size);

	arm_smccc_smc(EFUSE_OBJ_READ, obj_id, *size, 0, 0, 0, 0, 0, &res);

	rc = res.a0;
	len = res.a1;
	if (rc == EFUSE_OBJ_SUCCESS) {
		if (*size >= len) {
			memcpy(buff, (const void *)sharemem_output_base, len);
			*size = len;
		} else {
			rc = EFUSE_OBJ_ERR_SIZE;
		}
	}

	return rc;
}

uint32_t meson_efuse_obj_write(uint32_t obj_id, uint8_t *buff, uint32_t size)
{
	uint32_t rc = EFUSE_OBJ_ERR_UNKNOWN;
	struct arm_smccc_res res;

	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	memcpy((void *)sharemem_input_base, buff, size);

	arm_smccc_smc(EFUSE_OBJ_WRITE, obj_id, size, 0, 0, 0, 0, 0, &res);

	rc = res.a0;

	return rc;
}
#endif /* CONFIG_EFUSE_OBJ_API */

#ifdef CONFIG_EFUSE_MRK_GET_CHECKNUM
uint32_t meson_efuse_mrk_get_checknum(char *name, uint32_t *checknum)
{
	uint32_t rc = EFUSE_MRK_CHECKNUM_SUCCESS;
	struct arm_smccc_res res;

	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	strncpy((void *)sharemem_input_base, name, 16);
	arm_smccc_smc(EFUSE_MRK_GET_CHECKNUM, 0, 0, 0, 0, 0, 0, 0, &res);
	rc = res.a0;
	*checknum = res.a1;

	return rc;
}
#endif

int32_t meson_trustzone_efuse(struct efuse_hal_api_arg *arg)
{
	int ret;
	unsigned cmd, offset, size;
	unsigned long *retcnt = (unsigned long *)(arg->retcnt_phy);
	struct arm_smccc_res res;

	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);
	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	if (arg->cmd == EFUSE_HAL_API_READ)
		cmd = EFUSE_READ;
	else if (arg->cmd == EFUSE_HAL_API_READ_CALI)
		cmd = EFUSE_READ_CALI;
	else if (arg->cmd == EFUSE_HAL_API_WRITE)
		cmd = EFUSE_WRITE;
	else
		cmd = EFUSE_WRITE_PATTERN;
	offset = arg->offset;
	size = arg->size;

	if (arg->cmd == EFUSE_HAL_API_WRITE)
		memcpy((void *)sharemem_input_base,
		       (const void *)arg->buffer_phy, size);

	arm_smccc_smc(cmd, offset, size, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	*retcnt = res.a0;

	if ((arg->cmd == EFUSE_HAL_API_READ || arg->cmd == EFUSE_HAL_API_READ_CALI) && (ret != 0))
		memcpy((void *)arg->buffer_phy,
		       (const void *)sharemem_output_base, ret);

	if (!ret)
		return -1;
	else
		return 0;
}

int64_t __meson_trustzone_efuse_caliitem(uint32_t cmd, uint32_t subcmd)
{
	struct arm_smccc_res res;

	arm_smccc_smc(cmd, subcmd, subcmd, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

struct t_efuse_item {
	char *name;
	int  item;
};

const struct t_efuse_item efuselockitem_cfg[] = {
	{.name = "dgpk1", .item = EFUSE_LOCK_SUBITEM_DGPK1_KEY},
	{.name = "dgpk2", .item = EFUSE_LOCK_SUBITEM_DGPK2_KEY},
	{.name = "aud_id", .item = EFUSE_LOCK_SUBITEM_AUDIO_V_ID},
};

#define EFUSELOCKITEM_CNT   sizeof(efuselockitem_cfg) / sizeof(efuselockitem_cfg[0])
/*
 *return: 0: unlock, not write data
 *        1: lock, wrote data
 *        -1: fail
 */
int64_t meson_trustzone_efuse_lockitem(const char *str)
{
	int i;
	unsigned int subcmd = 0;
	int64_t ret = -1;

	for (i = 0; i < EFUSELOCKITEM_CNT; i++) {
		if (strncmp(efuselockitem_cfg[i].name, str,
			strlen(efuselockitem_cfg[i].name)) == 0) {
			subcmd = efuselockitem_cfg[i].item;
			break;
		}
	}
	if (i >= EFUSELOCKITEM_CNT)
		return -1;

	ret = __meson_trustzone_efuse_caliitem(EFUSE_READ_CALI_ITEM, subcmd);
	return ret;
}

int32_t meson_trustzone_efuse_get_max(struct efuse_hal_api_arg *arg)
{
	int32_t ret;
	unsigned cmd = 0;
	struct arm_smccc_res res;

	if (arg->cmd == EFUSE_HAL_API_USER_MAX)
		cmd = EFUSE_USER_MAX;
	else
		return -1;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);
	ret = res.a0;

	if (!ret)
		return -1;
	else
		return ret;
}

ssize_t meson_trustzone_efuse_writepattern(const char *buf, size_t count)
{
	struct efuse_hal_api_arg arg;
	unsigned long retcnt;

	if (count != EFUSE_BYTES)
		return 0;	/* Past EOF */

	arg.cmd = EFUSE_HAL_API_WRITE_PATTERN;
	arg.offset = 0;
	arg.size = count;
	arg.buffer_phy = (unsigned long)buf;
	arg.retcnt_phy = (unsigned long)&retcnt;
	int ret;

	ret = meson_trustzone_efuse(&arg);
	return ret;
}
#endif

uint64_t meson_trustzone_efuse_check(unsigned char *addr)
{
	struct sram_hal_api_arg arg = {};
	struct arm_smccc_res res;

	arg.cmd = SRAM_HAL_API_CHECK_EFUSE;
	arg.req_len = 0x1000000;
	arg.res_len = 0;
	arg.req_phy_addr = (unsigned long)addr;
	arg.res_phy_addr = (unsigned long)NULL;

	arm_smccc_smc(CALL_TRUSTZONE_HAL_API, TRUSTZONE_HAL_API_SRAM,
					(unsigned long)(&arg), 0, 0, 0, 0, 0, &res);

	return res.a0;
}

void debug_efuse_cmd(unsigned long cmd)
{
	struct arm_smccc_res res;

	arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);
}

void bl31_debug_efuse_write_pattern(const char *buf)
{
	if (!sharemem_input_base)
		sharemem_input_base =
			get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);
	memcpy((void *)sharemem_input_base, (const void *)buf, 512);

	debug_efuse_cmd(DEBUG_EFUSE_WRITE_PATTERN);
}

void bl31_debug_efuse_read_pattern(char *buf)
{
	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);
	debug_efuse_cmd(DEBUG_EFUSE_READ_PATTERN);

	memcpy((void *)buf, (const void *)sharemem_output_base, 512);
}

void aml_set_jtag_state(unsigned state, unsigned select)
{
	uint64_t command;
	struct arm_smccc_res res;

	if (state == JTAG_STATE_ON)
		command = JTAG_ON;
	else
		command = JTAG_OFF;
	arm_smccc_smc(command, select, 0, 0, 0, 0, 0, 0, &res);
}

void wdt_send_cmd_to_bl31(uint64_t cmd, uint64_t value)
{
	struct arm_smccc_res res;

	arm_smccc_smc(0x82000086, cmd, value, 0, 0, 0, 0, 0, &res);
}

unsigned aml_get_reboot_reason(void)
{
	unsigned int reason;
	struct arm_smccc_res res;

	arm_smccc_smc(GET_REBOOT_REASON, 0, 0, 0, 0, 0, 0, 0, &res);

	reason = (unsigned int)(res.a0 & 0xffffffff);

	return reason;
}

void set_viu_probe_enable(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(VIU_PREOBE_EN, 0, 0, 0, 0, 0, 0, 0, &res);
}

void power_set_dsp(unsigned int id, unsigned int powerflag)
{
	struct arm_smccc_res res;

	arm_smccc_smc(DSP_SEC_POWERSET, id, powerflag, 0, 0, 0, 0, 0, &res);
}

void init_dsp(unsigned int id,unsigned int addr,unsigned int cfg0)
{
	struct arm_smccc_res res;

	arm_smccc_smc(START_HIFI4, id, addr, cfg0, 0, 0, 0, 0, &res);
}


unsigned aml_reboot(uint64_t function_id, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);

	return (unsigned int)res.a0;
}

/* clear boot sequence related registers */
void aml_set_bootsequence(uint32_t val)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SET_STORAGE_BOOTSEQUENCE, val, 0, 0, 0, 0, 0, 0, &res);
}

void aml_set_reboot_reason(uint64_t function_id, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
}

unsigned long aml_sec_boot_check(unsigned long nType,
	unsigned long pBuffer,
	unsigned long nLength,
	unsigned long nOption)
{
	uint64_t ret = 1;
	struct arm_smccc_res res;

//#define AML_SECURE_LOG_TE

#if defined(AML_SECURE_LOG_TE)
	#define AML_GET_TE(a) do{a = *((volatile unsigned int*)0xc1109988);}while(0);
	unsigned nT1,nT2,nT3;
#else
	#define AML_GET_TE(...)
#endif

	AML_GET_TE(nT1);

	switch (nType) {
	case AML_D_P_W_EFUSE_AMLOGIC:
	case AML_D_P_W_EFUSE_SECURE_BOOT:
			break;
	default:
			return 0;
	}

	arm_smccc_smc(AML_DATA_PROCESS, nType, pBuffer, nLength, nOption, 0, 0, 0, &res);
	ret = res.a0;

	AML_GET_TE(nT2);

	flush_dcache_range((unsigned long )pBuffer, (unsigned long )pBuffer+nLength);

	AML_GET_TE(nT3);

#if defined(AML_SECURE_LOG_TE)
	printf("aml log : dec use %d(us) , flush cache used %d(us)\n",
		nT2 - nT1, nT3 - nT2);
#endif

	return ret;
}

void set_usb_boot_function(unsigned long command)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SET_USB_BOOT_FUNC, command, 0, 0, 0, 0, 0, 0, &res);
}

void set_boot_first_timeout(uint64_t arg0)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SET_BOOT_FIRST, arg0, 0, 0, 0, 0, 0, 0, &res);
}

void aml_system_off(void)
{
	/* TODO: Add poweroff capability */
	aml_reboot(0x82000042, 1, 0, 0);
	aml_reboot(0x84000008, 0, 0, 0);
}

int __get_chip_id(unsigned char *buff, unsigned int size)
{
	struct arm_smccc_res res;

	if (buff == NULL || size < 16)
		return -1;

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	if (sharemem_output_base) {
		arm_smccc_smc(GET_CHIP_ID, 2, 0, 0, 0, 0, 0, 0, &res);
		if (res.a0 == 0) {
			int version = *((unsigned int *)sharemem_output_base);

			if (version == 2) {
				memcpy(buff, (void *)sharemem_output_base + 4, 16);
			} else {
				/**
				 * Legacy 12-byte chip ID read out, transform data
				 * to expected order format.
				 */
				uint32_t chip_info = 0;//readl(P_AO_SEC_SD_CFG8);
				uint8_t *ch;
				int i;

				((uint32_t *)buff)[0] =
					((chip_info & 0xff000000)	|	// Family ID
					((chip_info << 8) & 0xff0000)	|	// Chip Revision
					((chip_info >> 8) & 0xff00));		// Package ID

				((uint32_t *)buff)[0] = htonl(((uint32_t *)buff)[0]);

				/* Transform into expected order for display */
				ch = (uint8_t *)(sharemem_output_base + 4);
				for (i = 0; i < 12; i++)
					buff[i + 4] = ch[11 - i];
			}

			return 0;
		}
	}

	return -1;
}

int bl31_get_cornerinfo(uint8_t *outbuf, int size)
{
	int buff_len = 0;
	struct arm_smccc_res res;

	if (outbuf == NULL) {
		printf("BL33: corner efuse info storebuf is NULL\n");
		return -1;
	}

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	if (sharemem_output_base) {
		arm_smccc_smc(OSCRING_EFUSE_GET, size, 0, 0, 0, 0, 0, 0, &res);
		buff_len = res.a0;
		if (buff_len <= size) {
			memcpy(outbuf, (void *)sharemem_output_base, buff_len);
			return 0;
		} else {
			printf("BL33: corner efuse info buf len %d over storebuf size %d\n", buff_len, size);
			return -1;
		}
	}
	return -1;
}

int32_t get_avbkey_from_fip(uint8_t *buf, uint32_t buflen)
{
	uint32_t retlen = 0;
	uint32_t ret = 0;
	struct arm_smccc_res res;

	if (!buf)
		return -1;

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	arm_smccc_smc(GET_AVBKEY_FROM_FIP, 0, 0, 0, 0, 0, 0, 0, &res);
	if (!res.a0)
		ret = -1;
	retlen = res.a0;

	if (ret != -1 && buflen >= retlen)
		memcpy(buf, (const void *)sharemem_output_base, retlen);

	return ret;
}

int32_t aml_get_bootloader_version(uint8_t *outbuf)
{
	struct arm_smccc_res res;

	if (!outbuf) {
		printf("BL33: get version storebuf is NULL\n");
		return -1;
	}

	if (!sharemem_output_base)
		sharemem_output_base =
			get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	if (sharemem_output_base) {
		arm_smccc_smc(GET_SHARE_MEM_INFORMATION, GET_SHARE_VERSION_DATA,
				0, 0, 0, 0, 0, 0, &res);
		memcpy(outbuf, (void *)sharemem_output_base, sizeof(build_messages_t));
		return 0;
	}

	return -1;
}
