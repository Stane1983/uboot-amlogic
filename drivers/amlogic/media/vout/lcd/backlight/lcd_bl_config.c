// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#ifdef CONFIG_AML_LCD_BL_LDIM
#include <amlogic/media/vout/lcd/bl_ldim.h>
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
#include <amlogic/media/vout/lcd/bl_extern.h>
#endif
#include "lcd_bl.h"
#include "../lcd_common.h"
#include "../lcd_unifykey.h"
#include "../lcd_reg.h"
#include "env.h"

struct bl_method_match_s {
	const char *name;
	enum bl_ctrl_method_e type;
};

static struct bl_method_match_s bl_method_match_table[] = {
	{"gpio",          BL_CTRL_GPIO},
	{"pwm",           BL_CTRL_PWM},
	{"pwm_combo",     BL_CTRL_PWM_COMBO},
	{"local_dimming", BL_CTRL_LOCAL_DIMMING},
	{"extern",        BL_CTRL_EXTERN},
	{"invalid",       BL_CTRL_MAX}
};

static const char *bl_method_type_to_str(int type)
{
	int i;
	const char *str = bl_method_match_table[BL_CTRL_MAX].name;

	for (i = 0; i < BL_CTRL_MAX; i++) {
		if (type == bl_method_match_table[i].type) {
			str = bl_method_match_table[i].name;
			break;
		}
	}
	return str;
}

void bl_config_print(struct aml_bl_drv_s *bdrv)
{
	struct bl_config_s *bconf = &bdrv->config;
	struct bl_pwm_config_s *bl_pwm;
#ifdef CONFIG_AML_LCD_BL_LDIM
	struct aml_ldim_driver_s *ldim_drv;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();
#endif

	BLPR("drv_index: %d\n", bdrv->index);
	BLPR("key_valid: %d\n", bdrv->key_valid);
	BLPR("state    : 0x%x\n", bdrv->state);
	BLPR("bl_off_policy: %d\n", bdrv->bl_off_policy);

	BLPR("name: %s\n", bconf->name);
	BLPR("method: %d\n", bconf->method);

	BLPR("level_default     = %d\n", bconf->level_default);
	BLPR("level_min         = %d\n", bconf->level_min);
	BLPR("level_max         = %d\n", bconf->level_max);
	BLPR("level_mid         = %d\n", bconf->level_mid);
	BLPR("level_mid_mapping = %d\n", bconf->level_mid_mapping);
	BLPR("level             = %d\n", bdrv->level);

	BLPR("en_gpio           = %s(%d)\n",
	     bconf->gpio_name[bconf->en_gpio], bconf->en_gpio);
	BLPR("en_gpio_on        = %d\n", bconf->en_gpio_on);
	BLPR("en_gpio_off       = %d\n", bconf->en_gpio_off);
	/* check if factory test */
	if (bdrv->factory_bl_on_delay >= 0)
		BLPR("factory test power_on_delay    = %d\n", bdrv->factory_bl_on_delay);
	else
		BLPR("power_on_delay    = %d\n", bconf->power_on_delay);
	BLPR("power_off_delay   = %d\n\n", bconf->power_off_delay);
	switch (bconf->method) {
	case BL_CTRL_PWM:
		if (bconf->bl_pwm) {
			bl_pwm = bconf->bl_pwm;
			BLPR("pwm_index     = %d\n", bl_pwm->index);
			BLPR("pwm_method    = %d\n", bl_pwm->pwm_method);
			BLPR("pwm_port      = %s(0x%x)\n",
			     bl_pwm_num_to_str(bl_pwm->pwm_port), bl_pwm->pwm_port);
			BLPR("bl_level_max      = %d\n", bl_pwm->bl_level_max);
			BLPR("bl_level_min      = %d\n", bl_pwm->bl_level_min);
			BLPR("bl_level          = %d\n", bl_pwm->bl_level);
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				BLPR("pwm_freq      = %d x vfreq\n", bl_pwm->pwm_freq);
				BLPR("pwm_phase      = %d\n", bl_pwm->pwm_phase);
				BLPR("pwm_cnt       = %u\n", bl_pwm->pwm_cnt);
				BLPR("pwm_duty      = %d\n", bl_pwm->pwm_duty);
				BLPR("pwm_reg0      = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V0));
				BLPR("pwm_reg1      = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V1));
				BLPR("pwm_reg2      = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V2));
				BLPR("pwm_reg3      = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V3));
			} else {
				BLPR("pwm_freq      = %uHz\n", bl_pwm->pwm_freq);
				BLPR("pwm_cnt       = %u\n", bl_pwm->pwm_cnt);
				BLPR("pwm_pre_div   = %u\n", bl_pwm->pwm_pre_div);
				BLPR("pwm_duty      = %d\n", bl_pwm->pwm_duty);
				bl_pwm_reg_print(bl_pwm);
			}
			BLPR("pwm_duty_range= %d\n", bl_pwm->pwm_duty_range);
			BLPR("pwm_duty_max  = %d\n", bl_pwm->pwm_duty_max);
			BLPR("pwm_duty_min  = %d\n", bl_pwm->pwm_duty_min);
			BLPR("pwm_gpio      = %s(%d)\n",
			     bconf->gpio_name[bl_pwm->pwm_gpio], bl_pwm->pwm_gpio);
			BLPR("pwm_gpio_off  = %d\n", bl_pwm->pwm_gpio_off);
		}
		BLPR("pwm_on_delay  = %d\n", bconf->pwm_on_delay);
		BLPR("pwm_off_delay = %d\n", bconf->pwm_off_delay);
		BLPR("en_sequence_reverse = %d\n", bconf->en_sequence_reverse);
		break;
	case BL_CTRL_PWM_COMBO:
		if (bconf->bl_pwm_combo0) {
			bl_pwm = bconf->bl_pwm_combo0;
			BLPR("pwm_combo0_index    = %d\n", bl_pwm->index);
			BLPR("pwm_combo0_method   = %d\n", bl_pwm->pwm_method);
			BLPR("pwm_combo0_port     = %s(0x%x)\n",
			     bl_pwm_num_to_str(bl_pwm->pwm_port), bl_pwm->pwm_port);
			BLPR("combo0_bl_level_max = %d\n", bl_pwm->bl_level_max);
			BLPR("combo0_bl_level_min = %d\n", bl_pwm->bl_level_min);
			BLPR("combo0_bl_level     = %d\n", bl_pwm->bl_level);
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				BLPR("pwm_combo0_freq  = %d x vfreq\n", bl_pwm->pwm_freq);
				BLPR("pwm_combo0_phase = %d\n", bl_pwm->pwm_phase);
				BLPR("pwm_combo0_cnt   = %u\n", bl_pwm->pwm_cnt);
				BLPR("pwm_combo0_duty  = %d\n", bl_pwm->pwm_duty);
				BLPR("pwm_combo0_reg0  = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V0));
				BLPR("pwm_combo0_reg1  = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V1));
				BLPR("pwm_combo0_reg2  = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V2));
				BLPR("pwm_combo0_reg3  = 0x%08x\n", lcd_vcbus_read(VPU_VPU_PWM_V3));
			} else {
				BLPR("pwm_combo0_freq    = %uHz\n", bl_pwm->pwm_freq);
				BLPR("pwm_combo0_cnt     = %u\n", bl_pwm->pwm_cnt);
				BLPR("pwm_combo0_pre_div = %u\n", bl_pwm->pwm_pre_div);
				BLPR("pwm_combo0_duty    = %d\n", bl_pwm->pwm_duty);
				bl_pwm_reg_print(bl_pwm);
			}
			BLPR("pwm_combo0_duty_range = %d\n", bl_pwm->pwm_duty_range);
			BLPR("pwm_combo0_duty_max = %d\n", bl_pwm->pwm_duty_max);
			BLPR("pwm_combo0_duty_min = %d\n", bl_pwm->pwm_duty_min);
			BLPR("pwm_combo0_gpio     = %s(%d)\n",
			     bconf->gpio_name[bl_pwm->pwm_gpio], bl_pwm->pwm_gpio);
			BLPR("pwm_combo0_gpio_off = %d\n", bl_pwm->pwm_gpio_off);
		}
		if (bconf->bl_pwm_combo1) {
			bl_pwm = bconf->bl_pwm_combo1;
			BLPR("pwm_combo1_index    = %d\n", bl_pwm->index);
			BLPR("pwm_combo1_method   = %d\n", bl_pwm->pwm_method);
			BLPR("pwm_combo1_port     = %s(0x%x)\n",
			     bl_pwm_num_to_str(bl_pwm->pwm_port), bl_pwm->pwm_port);
			BLPR("combo1_bl_level_max = %d\n", bl_pwm->bl_level_max);
			BLPR("combo1_bl_level_min = %d\n", bl_pwm->bl_level_min);
			BLPR("combo1_bl_level     = %d\n", bl_pwm->bl_level);
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				BLPR("pwm_combo1_freq    = %d x vfreq\n", bl_pwm->pwm_freq);
				BLPR("pwm_combo1_phase   = %d\n", bl_pwm->pwm_phase);
				BLPR("pwm_combo1_cnt     = %u\n", bl_pwm->pwm_cnt);
				BLPR("bl:pwm_combo1_duty = %d\n", bl_pwm->pwm_duty);
				BLPR("pwm_combo1_reg0    = 0x%08x\n",
				     lcd_vcbus_read(VPU_VPU_PWM_V0));
				BLPR("pwm_combo1_reg1    = 0x%08x\n",
				     lcd_vcbus_read(VPU_VPU_PWM_V1));
				BLPR("pwm_combo1_reg2    = 0x%08x\n",
				     lcd_vcbus_read(VPU_VPU_PWM_V2));
				BLPR("pwm_combo1_reg3    = 0x%08x\n",
				     lcd_vcbus_read(VPU_VPU_PWM_V3));
			} else {
				BLPR("pwm_combo1_freq    = %uHz\n", bl_pwm->pwm_freq);
				BLPR("pwm_combo1_cnt     = %u\n", bl_pwm->pwm_cnt);
				BLPR("pwm_combo1_pre_div = %u\n", bl_pwm->pwm_pre_div);
				BLPR("pwm_combo1_duty    = %d\n", bl_pwm->pwm_duty);

				bl_pwm_reg_print(bl_pwm);
			}
			BLPR("pwm_combo1_duty_range = %d\n", bl_pwm->pwm_duty_range);
			BLPR("pwm_combo1_duty_max = %d\n", bl_pwm->pwm_duty_max);
			BLPR("pwm_combo1_duty_min = %d\n", bl_pwm->pwm_duty_min);
			BLPR("pwm_combo1_gpio     = %s(%d)\n",
			     bconf->gpio_name[bl_pwm->pwm_gpio], bl_pwm->pwm_gpio);
			BLPR("pwm_combo1_gpio_off = %d\n", bl_pwm->pwm_gpio_off);
		}
		BLPR("pwm_on_delay        = %d\n", bconf->pwm_on_delay);
		BLPR("pwm_off_delay       = %d\n", bconf->pwm_off_delay);
		BLPR("en_sequence_reverse = %d\n", bconf->en_sequence_reverse);
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}
		ldim_drv = aml_ldim_get_driver();
		if (!ldim_drv) {
			BLPR("invalid local dimming driver\n");
			break;
		}
		if (ldim_drv->config_print)
			ldim_drv->config_print(ldim_drv);
		break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	case BL_CTRL_EXTERN:
		if (!bl_extern) {
			BLPR("invalid bl extern driver\n");
			break;
		}
		if (bl_extern->config_print)
			bl_extern->config_print();
		break;
#endif

	default:
		BLPR("invalid backlight control method\n");
		break;
	}
}

#ifdef CONFIG_OF_LIBFDT
static int bl_config_load_from_dts(char *dt_addr, struct aml_bl_drv_s *bdrv)
{
	int parent_offset, child_offset;
	char sname[20], propname[30];
	char *propdata;
	char *p;
	const char *str;
	struct bl_config_s *bconf = &bdrv->config;
	struct bl_pwm_config_s *bl_pwm;
	struct bl_pwm_config_s *pwm_combo0, *pwm_combo1;
	unsigned int temp;

	if (bdrv->index == 0)
		sprintf(sname, "/backlight");
	else
		sprintf(sname, "/backlight%d", bdrv->index);

	bconf->method = BL_CTRL_MAX; /* default */
	parent_offset = fdt_path_offset(dt_addr, sname);
	if (parent_offset < 0) {
		BLPR("not find %s node: %s\n", sname, fdt_strerror(parent_offset));
		return -1;
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (!propdata) {
		BLPR("not find status, default to disabled\n");
		return -1;
	}
	if (strncmp(propdata, "okay", 2)) {
		BLPR("status disabled\n");
		return -1;
	}

	sprintf(propname, "%s/backlight_%d", sname, bconf->index);
	child_offset = fdt_path_offset(dt_addr, propname);
	if (child_offset < 0) {
		BLERR("not find %s node: %s\n", propname, fdt_strerror(child_offset));
		return -1;
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "bl_name", NULL);
	if (!propdata) {
		BLERR("failed to get bl_name\n");
		sprintf(bconf->name, "backlight_%d", bconf->index);
	} else {
		strlcpy(bconf->name, propdata, BL_NAME_MAX);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset,
				       "bl_level_default_uboot_kernel", NULL);
	if (!propdata) {
		BLERR("failed to get bl_level_default_uboot_kernel\n");
		bconf->level_default = BL_LEVEL_DEFAULT;
	} else {
		bconf->level_default = be32_to_cpup((u32 *)propdata);
	}
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "bl_level_attr", NULL);
	if (!propdata) {
		BLERR("failed to get bl_level_attr\n");
		bconf->level_max = BL_LEVEL_MAX;
		bconf->level_min = BL_LEVEL_MIN;
		bconf->level_mid = BL_LEVEL_MID;
		bconf->level_mid_mapping = BL_LEVEL_MID_MAPPED;
	} else {
		bconf->level_max = be32_to_cpup((u32 *)propdata);
		bconf->level_min = be32_to_cpup((((u32 *)propdata) + 1));
		bconf->level_mid = be32_to_cpup((((u32 *)propdata) + 2));
		bconf->level_mid_mapping = be32_to_cpup((((u32 *)propdata) + 3));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "bl_ctrl_method", NULL);
	if (!propdata) {
		BLERR("failed to get bl_ctrl_method\n");
		bconf->method = BL_CTRL_MAX;
		return -1;
	}
	bconf->method = be32_to_cpup((u32 *)propdata);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "bl_power_attr", NULL);
	if (!propdata) {
		BLERR("failed to get bl_power_attr\n");
		bconf->en_gpio = BL_GPIO_NUM_MAX;
		bconf->en_gpio_on = LCD_GPIO_OUTPUT_HIGH;
		bconf->en_gpio_off = LCD_GPIO_OUTPUT_LOW;
		bconf->power_on_delay = 100;
		bconf->power_off_delay = 30;
	} else {
		bconf->en_gpio = be32_to_cpup((u32 *)propdata);
		bconf->en_gpio_on = be32_to_cpup((((u32 *)propdata) + 1));
		bconf->en_gpio_off = be32_to_cpup((((u32 *)propdata) + 2));
		bconf->power_on_delay = be32_to_cpup((((u32 *)propdata) + 3));
		bconf->power_off_delay = be32_to_cpup((((u32 *)propdata) + 4));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "en_sequence_reverse", NULL);
	if (!propdata)
		bconf->en_sequence_reverse = 0;
	else
		bconf->en_sequence_reverse = be32_to_cpup((u32 *)propdata);

	BLPR("[%d]: config from dts: %s: %s, method: %s(%d), en_seq_rev: %d\n",
	     bdrv->index, propname, bconf->name,
	     bl_method_type_to_str(bconf->method),
	     bconf->method, bconf->en_sequence_reverse);

	switch (bconf->method) {
	case BL_CTRL_PWM:
		if (!bconf->bl_pwm) {
			bconf->bl_pwm = (struct bl_pwm_config_s *)
				malloc(sizeof(struct bl_pwm_config_s));
			if (!bconf->bl_pwm) {
				BLERR("bl_pwm malloc error\n");
				return -1;
			}
		}
		bl_pwm = bconf->bl_pwm;
		memset(bl_pwm, 0, sizeof(struct bl_pwm_config_s));
		bl_pwm->index = 0;
		bl_pwm->drv_index = bdrv->index;

		bl_pwm->bl_level_max = bconf->level_max;
		bl_pwm->bl_level_min = bconf->level_min;

		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_port", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_port\n");
			bl_pwm->pwm_port = BL_PWM_MAX;
		} else {
			bl_pwm->pwm_port = bl_pwm_str_to_num(propdata);
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_attr", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_attr\n");
			bl_pwm->pwm_method = BL_PWM_POSITIVE;
			if (bl_pwm->pwm_port == BL_PWM_VS)
				bl_pwm->pwm_freq = BL_FREQ_VS_DEFAULT;
			else
				bl_pwm->pwm_freq = BL_FREQ_DEFAULT;
			bl_pwm->pwm_duty_max = 80;
			bl_pwm->pwm_duty_min = 20;
			bl_pwm->pwm_phase = 0;
		} else {
			bl_pwm->pwm_method = be32_to_cpup((u32 *)propdata);
			temp = be32_to_cpup((((u32 *)propdata) + 1));
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				bl_pwm->pwm_freq = temp & 0xff;
				bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;
			} else {
				bl_pwm->pwm_freq = temp;
				bl_pwm->pwm_phase = 0;
			}
			bl_pwm->pwm_duty_max =
				be32_to_cpup((((u32 *)propdata) + 2));
			bl_pwm->pwm_duty_min =
				be32_to_cpup((((u32 *)propdata) + 3));
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_power", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_power\n");
			bl_pwm->pwm_gpio = BL_GPIO_NUM_MAX;
			bl_pwm->pwm_gpio_off = LCD_GPIO_OUTPUT_LOW;
			bconf->pwm_on_delay = 10;
			bconf->pwm_off_delay = 10;
		} else {
			bl_pwm->pwm_gpio = be32_to_cpup((u32 *)propdata);
			bl_pwm->pwm_gpio_off =
				be32_to_cpup((((u32 *)propdata) + 1));
			bconf->pwm_on_delay =
				be32_to_cpup((((u32 *)propdata) + 2));
			bconf->pwm_off_delay =
				be32_to_cpup((((u32 *)propdata) + 3));
		}

		bl_pwm->pwm_duty = bl_pwm->pwm_duty_min;
		/* bl_pwm_config_init(bl_pwm); */
		break;
	case BL_CTRL_PWM_COMBO:
		if (!bconf->bl_pwm_combo0) {
			bconf->bl_pwm_combo0 = (struct bl_pwm_config_s *)
				malloc(sizeof(struct bl_pwm_config_s));
			if (!bconf->bl_pwm_combo0) {
				BLERR("bl_pwm_combo0 malloc error\n");
				return -1;
			}
		}
		if (!bconf->bl_pwm_combo1) {
			bconf->bl_pwm_combo1 = (struct bl_pwm_config_s *)
				malloc(sizeof(struct bl_pwm_config_s));
			if (!bconf->bl_pwm_combo1) {
				free(bconf->bl_pwm_combo0);
				BLERR("bl_pwm_combo1 struct malloc error\n");
				return -1;
			}
		}
		pwm_combo0 = bconf->bl_pwm_combo0;
		pwm_combo1 = bconf->bl_pwm_combo1;
		memset(pwm_combo0, 0, sizeof(struct bl_pwm_config_s));
		memset(pwm_combo1, 0, sizeof(struct bl_pwm_config_s));
		pwm_combo0->index = 0;
		pwm_combo1->index = 1;
		pwm_combo0->drv_index = bdrv->index;
		pwm_combo1->drv_index = bdrv->index;

		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_combo_level_mapping",
					       NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_combo_level_mapping\n");
			pwm_combo0->bl_level_max = BL_LEVEL_MAX;
			pwm_combo0->bl_level_min = BL_LEVEL_MID;
			pwm_combo1->bl_level_max = BL_LEVEL_MID;
			pwm_combo1->bl_level_min = BL_LEVEL_MIN;
		} else {
			pwm_combo0->bl_level_max = be32_to_cpup((u32 *)propdata);
			pwm_combo0->bl_level_min =
				be32_to_cpup((((u32 *)propdata) + 1));
			pwm_combo1->bl_level_max =
				be32_to_cpup((((u32 *)propdata) + 2));
			pwm_combo1->bl_level_min =
				be32_to_cpup((((u32 *)propdata) + 3));
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_combo_port", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_combo_port\n");
			pwm_combo0->pwm_port = BL_PWM_MAX;
			pwm_combo1->pwm_port = BL_PWM_MAX;
		} else {
			p = propdata;
			str = p;
			pwm_combo0->pwm_port = bl_pwm_str_to_num(str);
			p += strlen(p) + 1;
			str = p;
			pwm_combo1->pwm_port = bl_pwm_str_to_num(str);
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_combo_attr", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_combo_attr\n");
			pwm_combo0->pwm_method = BL_PWM_POSITIVE;
			if (pwm_combo0->pwm_port == BL_PWM_VS)
				pwm_combo0->pwm_freq = BL_FREQ_VS_DEFAULT;
			else
				pwm_combo0->pwm_freq = BL_FREQ_DEFAULT;
			pwm_combo0->pwm_duty_max = 80;
			pwm_combo0->pwm_duty_min = 20;
			pwm_combo0->pwm_phase = 0;
			pwm_combo1->pwm_method = BL_PWM_POSITIVE;
			if (pwm_combo1->pwm_port == BL_PWM_VS)
				pwm_combo1->pwm_freq = BL_FREQ_VS_DEFAULT;
			else
				pwm_combo1->pwm_freq = BL_FREQ_DEFAULT;
			pwm_combo1->pwm_duty_max = 80;
			pwm_combo1->pwm_duty_min = 20;
			pwm_combo1->pwm_phase = 0;
		} else {
			pwm_combo0->pwm_method = be32_to_cpup((u32 *)propdata);
			temp = be32_to_cpup((((u32 *)propdata) + 1));
			if (pwm_combo0->pwm_port == BL_PWM_VS) {
				pwm_combo0->pwm_freq = temp & 0xff;
				pwm_combo0->pwm_phase = (temp >> 8) & 0xffffff;
			} else {
				pwm_combo0->pwm_freq = temp;
				pwm_combo0->pwm_phase = 0;
			}
			pwm_combo0->pwm_duty_max =
				be32_to_cpup((((u32 *)propdata) + 2));
			pwm_combo0->pwm_duty_min =
				be32_to_cpup((((u32 *)propdata) + 3));
			pwm_combo1->pwm_method =
				be32_to_cpup((((u32 *)propdata) + 4));
			temp = be32_to_cpup((((u32 *)propdata) + 5));
			if (pwm_combo1->pwm_port == BL_PWM_VS) {
				pwm_combo1->pwm_freq = temp & 0xff;
				pwm_combo1->pwm_phase = (temp >> 8) & 0xffffff;
			} else {
				pwm_combo1->pwm_freq = temp;
				pwm_combo1->pwm_phase = 0;
			}
			pwm_combo1->pwm_duty_max =
				be32_to_cpup((((u32 *)propdata) + 6));
			pwm_combo1->pwm_duty_min =
				be32_to_cpup((((u32 *)propdata) + 7));
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset,
					       "bl_pwm_combo_power", NULL);
		if (!propdata) {
			BLERR("failed to get bl_pwm_combo_power\n");
			pwm_combo0->pwm_gpio = BL_GPIO_NUM_MAX;
			pwm_combo0->pwm_gpio_off = LCD_GPIO_INPUT;
			pwm_combo1->pwm_gpio = BL_GPIO_NUM_MAX;
			pwm_combo1->pwm_gpio_off = LCD_GPIO_INPUT;
			bconf->pwm_on_delay = 10;
			bconf->pwm_off_delay = 10;
		} else {
			pwm_combo0->pwm_gpio =
				be32_to_cpup((u32 *)propdata);
			pwm_combo0->pwm_gpio_off =
				be32_to_cpup((((u32 *)propdata) + 1));
			pwm_combo1->pwm_gpio =
				be32_to_cpup((((u32 *)propdata) + 2));
			pwm_combo1->pwm_gpio_off =
				be32_to_cpup((((u32 *)propdata) + 3));
			bconf->pwm_on_delay =
				be32_to_cpup((((u32 *)propdata) + 4));
			bconf->pwm_off_delay =
				be32_to_cpup((((u32 *)propdata) + 5));
		}

		pwm_combo0->pwm_duty = pwm_combo0->pwm_duty_min;
		pwm_combo1->pwm_duty = pwm_combo1->pwm_duty_min;
		/* bl_pwm_config_init(pwm_combo0);
		 *bl_pwm_config_init(pwm_combo1);
		 */
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}

		aml_ldim_probe(bdrv, dt_addr, child_offset, NULL, 0);
		break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	case BL_CTRL_EXTERN:
		/* get bl_extern_index from dts */
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "bl_extern_index", NULL);
		if (!propdata) {
			BLERR("failed to get bl_extern_index\n");
		} else {
			bconf->bl_extern_index = be32_to_cpup((u32 *)propdata);
			BLPR("get bl_extern_index = %d\n", bconf->bl_extern_index);
		}
		bl_extern_device_load(dt_addr, bconf->bl_extern_index);
		break;
#endif

	default:
		break;
	}

	return 0;
}
#endif

static int bl_config_load_from_ukey(char *dt_addr, struct aml_bl_drv_s *bdrv)
{
	unsigned char *para;
	int key_len, len;
	unsigned char *p;
	const char *str;
	char sname[20];
	struct lcd_unifykey_header_s *bl_header;
	struct bl_config_s *bconf = &bdrv->config;
	struct bl_pwm_config_s *bl_pwm;
	struct bl_pwm_config_s *pwm_combo0, *pwm_combo1;
	int ret;
	unsigned int temp;

	if (bdrv->index == 0)
		sprintf(sname, "backlight");
	else
		sprintf(sname, "backlight%d", bdrv->index);
	ret = lcd_unifykey_get_size(sname, &key_len);
	if (ret)
		return -1;
	para = (unsigned char *)malloc(key_len);
	if (!para) {
		BLERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(para, 0, key_len);

	ret = lcd_unifykey_get(sname, para, key_len);
	if (ret) {
		free(para);
		return -1;
	}

	/* step 1: check header */
	bl_header = (struct lcd_unifykey_header_s *)para;
	switch (bl_header->version) {
	case 2:
		len = 10 + 30 + 12 + 8 + 32 + 10;
		break;
	default:
		len = 10 + 30 + 12 + 8 + 32;
		break;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		lcd_unifykey_header_print(para);

	/* step 2: check backlight parameters */
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret) {
		BLERR("ukey length is incorrect\n");
		free(para);
		return -1;
	}

	/* basic: 30byte */
	p = para;
	str = (const char *)(p + LCD_UKEY_HEAD_SIZE);
	strlcpy(bconf->name, str, sizeof(bconf->name));
	bconf->name[sizeof(bconf->name) - 1] = '\0';

	/* level: 12byte */
	bconf->level_default = (*(p + LCD_UKEY_BL_LEVEL_UBOOT) |
		 ((*(p + LCD_UKEY_BL_LEVEL_UBOOT + 1)) << 8));
	bconf->level_max = (*(p + LCD_UKEY_BL_LEVEL_MAX) |
		((*(p + LCD_UKEY_BL_LEVEL_MAX + 1)) << 8));
	bconf->level_min = (*(p + LCD_UKEY_BL_LEVEL_MIN) |
		((*(p  + LCD_UKEY_BL_LEVEL_MIN + 1)) << 8));
	bconf->level_mid = (*(p + LCD_UKEY_BL_LEVEL_MID) |
		((*(p + LCD_UKEY_BL_LEVEL_MID + 1)) << 8));
	bconf->level_mid_mapping = (*(p + LCD_UKEY_BL_LEVEL_MID_MAP) |
		((*(p + LCD_UKEY_BL_LEVEL_MID_MAP + 1)) << 8));

	/* method: 8byte */
	bconf->method = *(p + LCD_UKEY_BL_METHOD);
	bconf->en_gpio = *(p + LCD_UKEY_BL_EN_GPIO);
	bconf->en_gpio_on = *(p + LCD_UKEY_BL_EN_GPIO_ON);
	bconf->en_gpio_off = *(p + LCD_UKEY_BL_EN_GPIO_OFF);
	bconf->power_on_delay = (*(p + LCD_UKEY_BL_ON_DELAY) |
		((*(p + LCD_UKEY_BL_ON_DELAY + 1)) << 8));
	bconf->power_off_delay = (*(p + LCD_UKEY_BL_OFF_DELAY) |
		((*(p + LCD_UKEY_BL_OFF_DELAY + 1)) << 8));

	if (bl_header->version == 2)
		bconf->en_sequence_reverse = (*(p + LCD_UKEY_BL_CUST_VAL_0) |
					((*(p + LCD_UKEY_BL_CUST_VAL_0 + 1)) << 8));
	else
		bconf->en_sequence_reverse = 0;

	BLPR("[%d]: config from ukey: %s, method: %s(%d), en_seq_rev: %d\n",
	     bdrv->index, bconf->name, bl_method_type_to_str(bconf->method),
	     bconf->method, bconf->en_sequence_reverse);

	/* pwm: 32byte */
	switch (bconf->method) {
	case BL_CTRL_PWM:
		bconf->bl_pwm = (struct bl_pwm_config_s *)
			malloc(sizeof(struct bl_pwm_config_s));
		if (!bconf->bl_pwm) {
			BLERR("bl_pwm struct malloc error\n");
			free(para);
			return -1;
		}
		bl_pwm = bconf->bl_pwm;
		bl_pwm->index = 0;
		bl_pwm->drv_index = bdrv->index;

		bl_pwm->bl_level_max = bconf->level_max;
		bl_pwm->bl_level_min = bconf->level_min;

		bconf->pwm_on_delay = (*(p + LCD_UKEY_BL_PWM_ON_DELAY) |
			((*(p + LCD_UKEY_BL_PWM_ON_DELAY + 1)) << 8));
		bconf->pwm_off_delay = (*(p + LCD_UKEY_BL_PWM_OFF_DELAY) |
			((*(p + LCD_UKEY_BL_PWM_OFF_DELAY + 1)) << 8));
		bl_pwm->pwm_method =  *(p + LCD_UKEY_BL_PWM_METHOD);
		bl_pwm->pwm_port = *(p + LCD_UKEY_BL_PWM_PORT);
		temp = (*(p + LCD_UKEY_BL_PWM_FREQ) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 1)) << 8) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 2)) << 8) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 3)) << 8));
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			bl_pwm->pwm_freq = temp & 0xff;
			bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;
		} else {
			bl_pwm->pwm_freq = temp;
			bl_pwm->pwm_phase = 0;
		}
		bl_pwm->pwm_duty_max = *(p + LCD_UKEY_BL_PWM_DUTY_MAX);
		bl_pwm->pwm_duty_min = *(p + LCD_UKEY_BL_PWM_DUTY_MIN);
		bl_pwm->pwm_gpio = *(p + LCD_UKEY_BL_PWM_GPIO);
		bl_pwm->pwm_gpio_off = *(p + LCD_UKEY_BL_PWM_GPIO_OFF);

		bl_pwm->pwm_duty = bl_pwm->pwm_duty_min;
		/* bl_pwm_config_init(bl_pwm); */
		break;
	case BL_CTRL_PWM_COMBO:
		bconf->bl_pwm_combo0 = (struct bl_pwm_config_s *)
			malloc(sizeof(struct bl_pwm_config_s));
		if (!bconf->bl_pwm_combo0) {
			BLERR("bl_pwm_combo0 struct malloc error\n");
			free(para);
			return -1;
		}
		bconf->bl_pwm_combo1 = (struct bl_pwm_config_s *)
				malloc(sizeof(struct bl_pwm_config_s));
		if (!bconf->bl_pwm_combo1) {
			BLERR("bl_pwm_combo1 struct malloc error\n");
			free(para);
			return -1;
		}
		pwm_combo0 = bconf->bl_pwm_combo0;
		pwm_combo1 = bconf->bl_pwm_combo1;
		pwm_combo0->index = 0;
		pwm_combo1->index = 1;
		pwm_combo0->drv_index = bdrv->index;
		pwm_combo1->drv_index = bdrv->index;

		bconf->pwm_on_delay = (*(p + LCD_UKEY_BL_PWM_ON_DELAY) |
			((*(p + LCD_UKEY_BL_PWM_ON_DELAY + 1)) << 8));
		bconf->pwm_off_delay = (*(p + LCD_UKEY_BL_PWM_OFF_DELAY) |
			((*(p + LCD_UKEY_BL_PWM_OFF_DELAY + 1)) << 8));
		pwm_combo0->pwm_method = *(p + LCD_UKEY_BL_PWM_METHOD);
		pwm_combo0->pwm_port = *(p + LCD_UKEY_BL_PWM_PORT);
		temp = (*(p + LCD_UKEY_BL_PWM_FREQ) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 1)) << 8) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 2)) << 8) |
			((*(p + LCD_UKEY_BL_PWM_FREQ + 3)) << 8));
		if (pwm_combo0->pwm_port == BL_PWM_VS) {
			pwm_combo0->pwm_freq = temp & 0xff;
			pwm_combo0->pwm_phase = (temp >> 8) & 0xffffff;
		} else {
			pwm_combo0->pwm_freq = temp;
			pwm_combo0->pwm_phase = 0;
		}
		pwm_combo0->pwm_duty_max = *(p + LCD_UKEY_BL_PWM_DUTY_MAX);
		pwm_combo0->pwm_duty_min = *(p + LCD_UKEY_BL_PWM_DUTY_MIN);
		pwm_combo0->pwm_gpio = *(p + LCD_UKEY_BL_PWM_GPIO);
		pwm_combo0->pwm_gpio_off = *(p + LCD_UKEY_BL_PWM_GPIO_OFF);
		pwm_combo1->pwm_method = *(p + LCD_UKEY_BL_PWM2_METHOD);
		pwm_combo1->pwm_port = *(p + LCD_UKEY_BL_PWM2_PORT);
		temp = (*(p + LCD_UKEY_BL_PWM2_FREQ) |
			((*(p + LCD_UKEY_BL_PWM2_FREQ + 1)) << 8) |
			((*(p + LCD_UKEY_BL_PWM2_FREQ + 2)) << 8) |
			((*(p + LCD_UKEY_BL_PWM2_FREQ + 3)) << 8));
		if (pwm_combo1->pwm_port == BL_PWM_VS) {
			pwm_combo1->pwm_freq = temp & 0xff;
			pwm_combo1->pwm_phase = (temp >> 8) & 0xffffff;
		} else {
			pwm_combo1->pwm_freq = temp;
			pwm_combo1->pwm_phase = 0;
		}
		pwm_combo1->pwm_duty_max = *(p + LCD_UKEY_BL_PWM2_DUTY_MAX);
		pwm_combo1->pwm_duty_min = *(p + LCD_UKEY_BL_PWM2_DUTY_MIN);
		pwm_combo1->pwm_gpio = *(p + LCD_UKEY_BL_PWM2_GPIO);
		pwm_combo1->pwm_gpio_off = *(p + LCD_UKEY_BL_PWM2_GPIO_OFF);

		pwm_combo0->bl_level_max = (*(p + LCD_UKEY_BL_PWM_LEVEL_MAX) |
			((*(p + LCD_UKEY_BL_PWM_LEVEL_MAX + 1)) << 8));
		pwm_combo0->bl_level_min = (*(p + LCD_UKEY_BL_PWM_LEVEL_MIN) |
			((*(p + LCD_UKEY_BL_PWM_LEVEL_MIN + 1)) << 8));
		pwm_combo1->bl_level_max = (*(p + LCD_UKEY_BL_PWM2_LEVEL_MAX) |
			((*(p + LCD_UKEY_BL_PWM2_LEVEL_MAX + 1)) << 8));
		pwm_combo1->bl_level_min = (*(p + LCD_UKEY_BL_PWM2_LEVEL_MIN) |
			((*(p + LCD_UKEY_BL_PWM2_LEVEL_MIN + 1)) << 8));

		pwm_combo0->pwm_duty = pwm_combo0->pwm_duty_min;
		pwm_combo1->pwm_duty = pwm_combo1->pwm_duty_min;
		/* bl_pwm_config_init(pwm_combo0);
		 *bl_pwm_config_init(pwm_combo1);
		 */
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}
		if (bl_header->version == 2) {
			aml_ldim_probe(bdrv, dt_addr, 0, para, 2);
		} else {
			BLERR("not support ldim for unifykey version: %d\n",
			      bl_header->version);
		}
		break;
#endif
	default:
		break;
	}

	free(para);
	return 0;
}

static int bl_config_load_from_bsp(struct aml_bl_drv_s *bdrv)
{
	struct ext_lcd_config_s *ext_lcd = NULL;
	char *panel_type, sname[20];
	unsigned int i = 0, j, done;
	char *str;
	struct bl_config_s *bconf = &bdrv->config;
	struct bl_pwm_config_s *bl_pwm;
	struct bl_pwm_config_s *pwm_combo0, *pwm_combo1;
	char (*bl_gpio)[LCD_CPU_GPIO_NAME_MAX];

	if (!bdrv->data)
		return -1;

	if (bdrv->index == 0)
		sprintf(sname, "panel_type");
	else
		sprintf(sname, "panel%d_type", bdrv->index);
	panel_type = env_get(sname);
	if (!panel_type) {
		BLERR("no %s\n", sname);
		return -1;
	}

	if (!bdrv->data->dft_conf[bdrv->index]) {
		BLERR("no dft_conf\n");
		return -1;
	}

	ext_lcd = bdrv->data->dft_conf[bdrv->index]->ext_lcd;
	if (!ext_lcd) {
		BLERR("%s: ext_lcd is NULL\n", __func__);
		return -1;
	}

	done = 0;
	for (i = 0; i < LCD_NUM_MAX; i++) {
		if (strcmp(ext_lcd->panel_type, panel_type) == 0) {
			done = 1;
			break;
		}
		if (strcmp(ext_lcd->panel_type, "invalid") == 0)
			break;
		ext_lcd++;
	}
	if (done == 0) {
		BLERR("can't find %s\n ", panel_type);
		return -1;
	}

	strlcpy(bconf->name, panel_type, sizeof(bconf->name));
	bconf->name[sizeof(bconf->name) - 1] = '\0';

	bconf->level_default     = ext_lcd->level_default;
	bconf->level_min         = ext_lcd->level_min;
	bconf->level_max         = ext_lcd->level_max;
	bconf->level_mid         = ext_lcd->level_mid;
	bconf->level_mid_mapping = ext_lcd->level_mid_mapping;

	bconf->method = ext_lcd->bl_method;

	if (ext_lcd->bl_en_gpio >= BL_GPIO_NUM_MAX)
		bconf->en_gpio = LCD_GPIO_MAX;
	else
		bconf->en_gpio = ext_lcd->bl_en_gpio;
	bconf->en_gpio_on      = ext_lcd->bl_en_gpio_on;
	bconf->en_gpio_off     = ext_lcd->bl_en_gpio_off;
	bconf->power_on_delay  = ext_lcd->bl_power_on_delay;
	bconf->power_off_delay = ext_lcd->bl_power_off_delay;

	switch (bconf->method) {
	case BL_CTRL_PWM:
		bl_pwm = (struct bl_pwm_config_s *)malloc(sizeof(struct bl_pwm_config_s));
		if (!bl_pwm) {
			BLERR("bl_pwm struct malloc error\n");
			return -1;
		}
		bconf->bl_pwm = bl_pwm;
		bl_pwm->index = 0;
		bl_pwm->drv_index = bdrv->index;

		bl_pwm->bl_level_max  = bconf->level_max;
		bl_pwm->bl_level_min  = bconf->level_min;

		bl_pwm->pwm_method    = ext_lcd->pwm_method;
		bl_pwm->pwm_port      = ext_lcd->pwm_port;
		bl_pwm->pwm_freq      = ext_lcd->pwm_freq;
		bl_pwm->pwm_duty_max  = ext_lcd->pwm_duty_max;
		bl_pwm->pwm_duty_min  = ext_lcd->pwm_duty_min;

		bl_pwm->pwm_gpio      = ext_lcd->pwm_gpio;
		bl_pwm->pwm_gpio_off  = ext_lcd->pwm_gpio_off;
		bconf->pwm_on_delay   = ext_lcd->pwm_on_delay;
		bconf->pwm_off_delay  = ext_lcd->pwm_off_delay;

		bl_pwm->pwm_duty = bl_pwm->pwm_duty_min;
		/* bl_pwm_config_init(bl_pwm); */
		break;
	case BL_CTRL_PWM_COMBO:
		bconf->bl_pwm_combo0 = (struct bl_pwm_config_s *)
			malloc(sizeof(struct bl_pwm_config_s));
		if (!bconf->bl_pwm_combo0) {
			BLERR("bl_pwm_combo0 struct malloc error\n");
			return -1;
		}
		bconf->bl_pwm_combo1 = (struct bl_pwm_config_s *)
			malloc(sizeof(struct bl_pwm_config_s));
		if (!bconf->bl_pwm_combo1) {
			BLERR("bl_pwm_combo1 struct malloc error\n");
			return -1;
		}
		pwm_combo0 = bconf->bl_pwm_combo0;
		pwm_combo1 = bconf->bl_pwm_combo1;
		pwm_combo0->index = 0;
		pwm_combo1->index = 1;
		pwm_combo0->drv_index = bdrv->index;
		pwm_combo1->drv_index = bdrv->index;

		pwm_combo0->bl_level_max  = ext_lcd->pwm_level_max;
		pwm_combo0->bl_level_min  = ext_lcd->pwm_level_min;
		pwm_combo1->bl_level_max  = ext_lcd->pwm2_level_max;
		pwm_combo1->bl_level_min  = ext_lcd->pwm2_level_min;

		pwm_combo0->pwm_method    = ext_lcd->pwm_method;
		pwm_combo0->pwm_port      = ext_lcd->pwm_port;
		pwm_combo0->pwm_freq      = ext_lcd->pwm_freq;
		pwm_combo0->pwm_duty_max  = ext_lcd->pwm_duty_max;
		pwm_combo0->pwm_duty_min  = ext_lcd->pwm_duty_min;
		if (ext_lcd->pwm_gpio >= BL_GPIO_NUM_MAX) {
			pwm_combo0->pwm_gpio = LCD_GPIO_MAX;
		} else {
			str = bconf->gpio_name[ext_lcd->pwm_gpio];
			pwm_combo0->pwm_gpio = lcd_gpio_name_map_num(str);
		}
		pwm_combo0->pwm_gpio_off  = ext_lcd->pwm_gpio_off;
		pwm_combo1->pwm_method    = ext_lcd->pwm2_method;
		pwm_combo1->pwm_port      = ext_lcd->pwm2_port;
		pwm_combo1->pwm_freq      = ext_lcd->pwm2_freq;
		pwm_combo1->pwm_duty_max  = ext_lcd->pwm2_duty_max;
		pwm_combo1->pwm_duty_min  = ext_lcd->pwm2_duty_min;
		if (ext_lcd->pwm2_gpio >= BL_GPIO_NUM_MAX) {
			pwm_combo1->pwm_gpio = LCD_GPIO_MAX;
		} else {
			str = bconf->gpio_name[ext_lcd->pwm2_gpio];
			pwm_combo1->pwm_gpio = lcd_gpio_name_map_num(str);
		}
		pwm_combo1->pwm_gpio_off  = ext_lcd->pwm2_gpio_off;
		bconf->pwm_on_delay   = ext_lcd->pwm_on_delay;
		bconf->pwm_off_delay  = ext_lcd->pwm_off_delay;

		pwm_combo0->pwm_duty = pwm_combo0->pwm_duty_min;
		pwm_combo1->pwm_duty = pwm_combo1->pwm_duty_min;
		/* bl_pwm_config_init(pwm_combo0);
		 *bl_pwm_config_init(pwm_combo1);
		 */
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}
		aml_ldim_probe(bdrv, NULL, 0, NULL, 1);
		break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	case BL_CTRL_EXTERN:
		bl_extern_device_load(NULL, bconf->bl_extern_index);
		break;
#endif
	default:
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
			BLPR("invalid backlight control method\n");
		break;
	}

	i = 0;
	bl_gpio = bdrv->data->dft_conf[bdrv->index]->bl_gpio;
	if (!bl_gpio) {
		LCDERR("%s: bl_gpio is null\n", __func__);
		return -1;
	}
	while (i < BL_GPIO_NUM_MAX) {
		if (strcmp(bl_gpio[i], "invalid") == 0)
			break;
		strcpy(bconf->gpio_name[i], bl_gpio[i]);
		i++;
	}
	for (j = i; j < BL_GPIO_NUM_MAX; j++)
		strcpy(bconf->gpio_name[j], "invalid");

	return 0;
}

static char *bl_pinmux_str[] = {
	"bl_pwm_on_pin",        /* 0 */
	"bl_pwm_vs_on_pin",     /* 1 */
	"bl_pwm_combo_0_on_pin",  /* 2 */
	"bl_pwm_combo_1_on_pin",  /* 3 */
	"bl_pwm_combo_0_vs_on_pin",  /* 4 */
	"bl_pwm_combo_1_vs_on_pin",  /* 5 */
};

static int bl_pinmux_load_from_bsp(struct aml_bl_drv_s *bdrv)
{
	struct bl_config_s *bconf = &bdrv->config;
	char propname[50];
	struct lcd_pinmux_ctrl_s *pinmux;
	unsigned int i, j;
	int pinmux_index = 0, set_cnt = 0, clr_cnt = 0;
	struct bl_pwm_config_s *bl_pwm;
	struct bl_pwm_config_s *pwm_combo0, *pwm_combo1;

	if (!bdrv->data)
		return -1;
	if (!bdrv->data->dft_conf[bdrv->index]) {
		BLERR("%s: dft_conf is NULL\n", __func__);
		return -1;
	}
	bconf->bl_pinmux = bdrv->data->dft_conf[bdrv->index]->bl_pinmux;
	if (!bconf->bl_pinmux) {
		BLERR("%s: bl_pinmux is NULL\n", __func__);
		return -1;
	}

	switch (bconf->method) {
	case BL_CTRL_PWM:
		bl_pwm = bconf->bl_pwm;
		if (bl_pwm->pwm_port == BL_PWM_VS)
			pinmux_index = 1;
		else
			pinmux_index = 0;
		sprintf(propname, "%s", bl_pinmux_str[pinmux_index]);
		pinmux = bconf->bl_pinmux;
		for (i = 0; i < LCD_PINMX_MAX; i++) {
			if (!pinmux)
				break;
			if (!pinmux->name)
				break;
			if (strncmp(pinmux->name, "invalid", 7) == 0)
				break;
			if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
						break;
					bl_pwm->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
					bl_pwm->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
					set_cnt++;
				}
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
						break;
					bl_pwm->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
					bl_pwm->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
					clr_cnt++;
				}
				break;
			}
			pinmux++;
		}
		if (set_cnt < LCD_PINMUX_NUM) {
			bl_pwm->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
			bl_pwm->pinmux_set[set_cnt][1] = 0x0;
		}
		if (clr_cnt < LCD_PINMUX_NUM) {
			bl_pwm->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
			bl_pwm->pinmux_clr[clr_cnt][1] = 0x0;
		}

		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (bl_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
					break;
				BLPR("bl_pinmux set: %d, 0x%08x\n",
				     bl_pwm->pinmux_set[i][0], bl_pwm->pinmux_set[i][1]);
				i++;
			}
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				BLPR("bl_pinmux clr: %d, 0x%08x\n",
				     bl_pwm->pinmux_clr[i][0], bl_pwm->pinmux_clr[i][1]);
				i++;
			}
		}
		break;
	case BL_CTRL_PWM_COMBO:
		pwm_combo0 = bconf->bl_pwm_combo0;
		pwm_combo1 = bconf->bl_pwm_combo1;
		if (pwm_combo0->pwm_port == BL_PWM_VS)
			sprintf(propname, "%s", bl_pinmux_str[4]);
		else
			sprintf(propname, "%s", bl_pinmux_str[2]);

		pinmux = bconf->bl_pinmux;
		for (i = 0; i < LCD_PINMX_MAX; i++) {
			if (!pinmux)
				break;
			if (!pinmux->name)
				break;
			if (strncmp(pinmux->name, "invalid", 7) == 0)
				break;
			if (strncmp(pinmux->name, propname,
				    strlen(propname)) == 0) {
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
						break;
					pwm_combo0->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
					pwm_combo0->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
					set_cnt++;
				}
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
						break;
					pwm_combo0->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
					pwm_combo0->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
					clr_cnt++;
				}
				break;
			}
			pinmux++;
		}
		if (set_cnt < LCD_PINMUX_NUM) {
			pwm_combo0->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
			pwm_combo0->pinmux_set[set_cnt][1] = 0x0;
		}
		if (clr_cnt < LCD_PINMUX_NUM) {
			pwm_combo0->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
			pwm_combo0->pinmux_clr[clr_cnt][1] = 0x0;
		}

		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (pwm_combo0->pinmux_set[i][0] == LCD_PINMUX_END)
					break;
				BLPR("pwm_combo0 pinmux_set: %d, 0x%08x\n",
				     pwm_combo0->pinmux_set[i][0], pwm_combo0->pinmux_set[i][1]);
				i++;
			}
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (pwm_combo0->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				BLPR("pwm_combo0 pinmux_clr: %d, 0x%08x\n",
				     pwm_combo0->pinmux_clr[i][0], pwm_combo0->pinmux_clr[i][1]);
				i++;
			}
		}

		if (pwm_combo1->pwm_port == BL_PWM_VS)
			sprintf(propname, "%s", bl_pinmux_str[5]);
		else
			sprintf(propname, "%s", bl_pinmux_str[3]);

		pinmux = bconf->bl_pinmux;
		set_cnt = 0;
		clr_cnt = 0;
		for (i = 0; i < LCD_PINMX_MAX; i++) {
			if (!pinmux)
				break;
			if (!pinmux->name)
				break;
			if (strncmp(pinmux->name, "invalid", 7) == 0)
				break;
			if (strncmp(pinmux->name, propname,
				    strlen(propname)) == 0) {
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
						break;
					pwm_combo1->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
					pwm_combo1->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
					set_cnt++;
				}
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
						break;
					pwm_combo1->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
					pwm_combo1->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
					clr_cnt++;
				}
				break;
			}
			pinmux++;
		}
		if (set_cnt < LCD_PINMUX_NUM) {
			pwm_combo1->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
			pwm_combo1->pinmux_set[set_cnt][1] = 0x0;
		}
		if (clr_cnt < LCD_PINMUX_NUM) {
			pwm_combo1->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
			pwm_combo1->pinmux_clr[clr_cnt][1] = 0x0;
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (pwm_combo1->pinmux_set[i][0] == LCD_PINMUX_END)
					break;
				BLPR("pwm_combo1 pinmux_set: %d, 0x%08x\n",
				     pwm_combo1->pinmux_set[i][0], pwm_combo1->pinmux_set[i][1]);
				i++;
			}
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (pwm_combo1->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				BLPR("pwm_combo1 pinmux_clr: %d, 0x%08x\n",
				     pwm_combo1->pinmux_clr[i][0], pwm_combo1->pinmux_clr[i][1]);
				i++;
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

/* config from json =============================================================================*/
#ifdef CONFIG_AML_LCD_JSON
static struct num_str_s bl_ctrl_method[] = {
	{BL_CTRL_GPIO,          "BL_CTRL_GPIO"},
	{BL_CTRL_PWM,           "BL_CTRL_PWM"},
	{BL_CTRL_PWM_COMBO,     "BL_CTRL_PWM_COMBO"},
	{BL_CTRL_LOCAL_DIMMING, "BL_CTRL_LOCAL_DIMMING"},
	{BL_CTRL_EXTERN,        "BL_CTRL_EXTERN"},
	{BL_CTRL_MAX,           "BL_CTRL_MAX"},
};

static inline int bl_ctrl_method_str2num(const char *str)
{
	return strnum_get_num(str, bl_ctrl_method, ARRAY_SIZE(bl_ctrl_method), BL_CTRL_MAX);
}

static int bl_gpio_name_to_index(struct aml_bl_drv_s *bdrv, const char *name)
{
	int i = 0;

	if (!bdrv || !name)
		return LCD_GPIO_MAX;

	for (i = 0; i < BL_GPIO_NUM_MAX; i++)
		if (!strcmp(bdrv->config.gpio_name[i], name))
			return i;
	return LCD_GPIO_MAX;
}

int bl_config_load_from_json(struct aml_bl_drv_s *bdrv)
{
	int index = 0;
	int cnt = 0, i = 0, ret = 0;
	struct json_parse_s *jsp;
	struct bl_config_s *bconf = &bdrv->config;
	struct bl_pwm_config_s *bl_pwm, *pwms[3] = {NULL, NULL, NULL};
	const char *str = NULL;
	struct json_s *parent, *child, *child2, *child3;

	index = bdrv->index;
	jsp = get_panel_jsp(index);

	if (!json_parse_ok(jsp)) {
		ret = panel_json_parse(jsp, get_panel_file(index, NULL));
		if (ret) {
			rm_panel_file(index);
			return -1;
		}
	}

	parent = json_get_object_child(jsp, jsp->root, "backlight");
	if (!parent) {
		BLERR("failed find /backlight\n");
		return -1;
	}

//basic
	child = json_get_object_child(jsp, parent, "basic_info");
	if (!child) {
		BLERR("failed find basic_info\n");
		return -1;
	}

	str = json_get_obj_str(jsp, child, "name", NULL);
	if (str)
		strncpy(bconf->name, str, BL_NAME_MAX - 1);

//level setup
	child = json_get_object_child(jsp, parent, "level_setup");
	if (!child) {
		BLERR("failed find level_setup\n");
		return -1;
	}

	child2 = json_get_object_child(jsp, child, "range");
	bconf->level_min         = json_get_arr_u32(jsp, child2, 0, BL_LEVEL_MIN);
	bconf->level_max         = json_get_arr_u32(jsp, child2, 1, BL_LEVEL_MAX);
	bconf->level_mid         = json_get_obj_u32(jsp, child, "mid", BL_LEVEL_MID);
	bconf->level_mid_mapping = json_get_obj_u32(jsp, child, "mid_mapping", BL_LEVEL_MID_MAPPED);
	bconf->level_default     = json_get_obj_u32(jsp, child, "uboot", BL_LEVEL_DEFAULT);

//control method
	child = json_get_object_child(jsp, parent, "control_method");
	if (!child) {
		BLERR("failed find control_method\n");
		return -1;
	}
	bconf->method  = bl_ctrl_method_str2num(json_get_obj_str(jsp, child, "method", NULL));
	bconf->en_gpio = bl_gpio_name_to_index(bdrv, json_get_obj_str(jsp, child, "en_gpio", NULL));
	bconf->en_gpio_on          = json_get_obj_u32(jsp, child, "en_gpio_on", 1);
	bconf->en_gpio_off         = json_get_obj_u32(jsp, child, "en_gpio_off", 0);
	//bconf->power_on_delay    = json_get_obj_u32(jsp, child, "bl_on_delay_ms", 0);
	//bconf->power_off_delay   = json_get_obj_u32(jsp, child, "bl_off_delay_ms", 0);
	bconf->pwm_on_delay        = json_get_obj_u32(jsp, child, "pwm_on_delay_ms", 0);
	bconf->pwm_off_delay       = json_get_obj_u32(jsp, child, "pwm_off_delay_ms", 0);
	bconf->en_sequence_reverse = json_get_obj_u32(jsp, child, "en_sequence_reverse", 0);

	if (bconf->method == BL_CTRL_LOCAL_DIMMING) {
#ifdef CONFIG_AML_LCD_BL_LDIM
		if (bdrv->index == 0)
			return aml_ldim_probe(bdrv, lcd_get_dt_addr(), 0, NULL, LCD_CONFIG_FILE);
		else
			return -1;
#else
		BLERR("%s not support ldim\n", __func__);
		return -1;
#endif
	}

//pwms
	if (bconf->method != BL_CTRL_PWM && bconf->method != BL_CTRL_PWM_COMBO)
		return 0;

	child = json_get_object_child(jsp, child, "pwms");
	if (!child) {
		BLERR("failed find pwms\n");
		return -1;
	}
	cnt = json_get_array_size(jsp, child);
	cnt = lcd_s32_constraint(cnt, 0, 2);
	for (i = 0; i < cnt; i++) {
		child2 = json_get_array_child(jsp, child, i);
		if (!child2) {
			BLPR("fail find pwm[%d]\n", i);
			for (i--; i >= 0; i--) {
				free(pwms[i]);
				pwms[i] = NULL;
			}
			return -1;
		}

		pwms[i] = (struct bl_pwm_config_s *)malloc(sizeof(*bl_pwm));
		if (!pwms[i]) {
			BLPR("error malloc bl_pwm\n");
			for (i--; i >= 0; i--) {
				free(pwms[i]);
				pwms[i] = NULL;
			}
			return -1;
		}

		bl_pwm = pwms[i];
		bl_pwm->drv_index = bdrv->index;
		bl_pwm->index = i;

		str = json_get_obj_str(jsp, child2, "port", NULL);
		bl_pwm->pwm_port      = bl_pwm_str_to_num(str ? str : "invalid");
		bl_pwm->pwm_method    = json_get_obj_u32(jsp, child2, "polarity", 1);
		bl_pwm->pwm_phase     = json_get_obj_u32(jsp, child2, "phase", 0);
		bl_pwm->pwm_freq      = json_get_obj_u32(jsp, child2, "freq", 180);
		str = json_get_obj_str(jsp, child2, "gpio", NULL);
		bl_pwm->pwm_gpio      = bl_gpio_name_to_index(bdrv, str);
		bl_pwm->pwm_gpio_off  = json_get_obj_u32(jsp, child2, "gpio_off", 0);

		if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
			bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;

		child3 = json_get_object_child(jsp, child2, "level_range");
		if (!child3)
			BLPR("failed find pwms[%d]/level_range\n", i);
		bl_pwm->bl_level_min = json_get_arr_u32(jsp, child3, 0, bconf->level_min);
		bl_pwm->bl_level_max = json_get_arr_u32(jsp, child3, 1, bconf->level_max);

		child3 = json_get_object_child(jsp, child2, "duty_range");
		if (!child3)
			BLPR("failed find pwms[%d]/level_range\n", i);
		bl_pwm->pwm_duty_min = json_get_arr_u32(jsp, child3, 0, 0);
		bl_pwm->pwm_duty_max = json_get_arr_u32(jsp, child3, 1, 100);
		bl_pwm->pwm_duty = json_get_obj_u32(jsp, child2, "duty", bl_pwm->pwm_duty_min);
	}

	bconf->bl_pwm = pwms[0];
	bconf->bl_pwm_combo0 = pwms[0];
	bconf->bl_pwm_combo1 = pwms[1];
	return 0;
}

#else
static int bl_config_load_from_json(struct aml_bl_drv_s *bdrv)
{
	return -1;
}
#endif

static unsigned int lcd_bl_dt_valid(char *dt_addr, int index)
{
#ifdef CONFIG_OF_LIBFDT
	int parent_offset;
	char str[16];
	char *propdata;

	if (index == 0)
		sprintf(str, "/backlight");
	else
		sprintf(str, "/backlight%d", index);

	parent_offset = fdt_path_offset(dt_addr, str);
	if (!parent_offset)
		return 0;
	/* check lcd status enable or not */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (propdata && strncmp(propdata, "okay", 2) == 0)
		return 1;

	LCDERR("[%d]: backlight disabled\n", index);
#endif
	return 0;
}

int bl_check_config_load(struct aml_bl_drv_s *bdrv)
{
	int ret = 0, dt_sta;

	dt_sta = lcd_bl_dt_valid(lcd_get_dt_addr(), bdrv->index);
	bdrv->config_load = lcd_panel_config_load_detect(bdrv->index, dt_sta, bdrv->key_valid);
	if (bdrv->config_load == LCD_CONFIG_NONE) {
		LCDERR("[%d] config_load_check error: config_load:%d, dt_status:%d, key:%d",
			bdrv->index, bdrv->config_load, dt_sta, bdrv->key_valid);
		return -1;
	}

	return ret;
}

static int bl_config_load(char *dt_addr, int load_id, struct aml_bl_drv_s *bdrv)
{
	char *bl_off_policy_str, str[30];
	unsigned int temp;
	int ret = -1;
	unsigned char file_type = PANEL_FILE_INVILD;

	bdrv->state = 0;

	if (bl_check_config_load(bdrv))
		return -1;

	switch (bdrv->config_load) {
	case LCD_CONFIG_FILE:
		file_type = get_lcd_panel_file_type(bdrv->index);
		if (file_type == PANEL_FILE_JSON)
			ret = bl_config_load_from_json(bdrv);
		else if (file_type == PANEL_FILE_INI)
			ret = -1; //todo
		break;
	case LCD_CONFIG_UKEY:
		ret = bl_config_load_from_ukey(dt_addr, bdrv);
		break;
	case LCD_CONFIG_DTS:
		ret = bl_config_load_from_dts(dt_addr, bdrv);
		break;
	case LCD_CONFIG_BSP:
		ret = bl_config_load_from_bsp(bdrv);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret) {
		bdrv->config.method = BL_CTRL_MAX;
		BLPR("[%d]: invalid backlight config\n", bdrv->index);
		return -1;
	}
	bl_pinmux_load_from_bsp(bdrv);
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		bl_config_print(bdrv);

	/* get bl_off_policy */
	bdrv->bl_off_policy = BL_OFF_POLICY_NONE;
	if (bdrv->index == 0)
		sprintf(str, "bl_off");
	else
		sprintf(str, "bl%d_off", bdrv->index);
	bl_off_policy_str = env_get(str);
	if (bl_off_policy_str) {
		if (strncmp(bl_off_policy_str, "none", 2) == 0)
			bdrv->bl_off_policy = BL_OFF_POLICY_NONE;
		else if (strncmp(bl_off_policy_str, "always", 2) == 0)
			bdrv->bl_off_policy = BL_OFF_POLICY_ALWAYS;
		else if (strncmp(bl_off_policy_str, "once", 2) == 0)
			bdrv->bl_off_policy = BL_OFF_POLICY_ONCE;
		BLPR("[%d]: bl_off_policy: %s\n", bdrv->index, bl_off_policy_str);
	}

	/* get bl_level */
	if (bdrv->index == 0)
		sprintf(str, "bl_level");
	else
		sprintf(str, "bl%d_level", bdrv->index);
	temp = env_get_ulong(str, 10, 0xffff);
	if (temp != 0xffff) {
		bdrv->config.level_default = temp;
		BLPR("[%d]: bl_level: %d\n", bdrv->index, bdrv->config.level_default);
	}

	/* get factory_bl_on_delay */
	if (bdrv->index == 0)
		sprintf(str, "factory_bl_on_delay");
	else
		sprintf(str, "factory_bl%d_on_delay", bdrv->index);
	temp = env_get_ulong(str, 10, 0xffff);
	if (temp != 0xffff) {
		bdrv->factory_bl_on_delay = temp;
		BLPR("[%d]: factory_bl_on_delay: %d\n", bdrv->index, bdrv->factory_bl_on_delay);
	}

	return 0;
}

static int lcd_bl_init_load_from_dts(char *dtaddr, struct aml_bl_drv_s *bdrv)
{
#ifdef CONFIG_OF_LIBFDT
	int parent_offset;
	char *propdata, *p, snode[15];
	const char *str;
	int i, j;
	int ret = 0;

	if (bdrv->index == 0)
		sprintf(snode, "/backlight");
	else
		sprintf(snode, "/backlight%d", bdrv->index);

	/* check key_valid */
	parent_offset = fdt_path_offset(dtaddr, snode);
	if (parent_offset < 0) {
		BLERR("not find %s node: %s\n", snode, fdt_strerror(parent_offset));
		return -1;
	}
	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "key_valid", NULL);
	if (!propdata) {
		BLERR("failed to get key_valid\n");
		bdrv->key_valid = 0;
	} else {
		bdrv->key_valid = be32_to_cpup((u32 *)propdata);
	}

	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "status", NULL);
	if (!propdata) {
		BLPR("failed to get status, default to disabled\n");
		return -1;
	}
	if (strncmp(propdata, "okay", 2)) {
		BLPR("status disabled\n");
		return -1;
	}

	/* gpio */
	i = 0;
	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "bl_gpio_names", NULL);
	if (!propdata) {
		BLERR("failed to get bl_gpio_names\n");
	} else {
		p = propdata;
		while (i < BL_GPIO_NUM_MAX) {
			if (i > 0)
				p += strlen(p) + 1;
			str = p;
			if (strlen(str) == 0)
				break;
			strlcpy(bdrv->config.gpio_name[i], str, LCD_CPU_GPIO_NAME_MAX);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				BLPR("i=%d, gpio=%s\n", i, bdrv->config.gpio_name[i]);
			i++;
		}
	}

	for (j = i; j < BL_GPIO_NUM_MAX; j++)
		strcpy(bdrv->config.gpio_name[j], "invalid");

	return ret;
#elif
	return -1;
#endif
}

static int lcd_bl_init_load_from_bsp(struct aml_bl_drv_s *bdrv)
{
	return 0;
}

int aml_bl_load_config(struct aml_bl_drv_s *bdrv, char *dt_addr, int load_id)
{
	int ret;

	if (!bdrv || !dt_addr)
		return -1;

	if (load_id != LCD_CONFIG_BSP)
		ret = lcd_bl_init_load_from_dts(dt_addr, bdrv);
	else
		ret = lcd_bl_init_load_from_bsp(bdrv);
	if (ret)
		return -1;

	/* load bl config */
	return bl_config_load(dt_addr, load_id, bdrv);
}
