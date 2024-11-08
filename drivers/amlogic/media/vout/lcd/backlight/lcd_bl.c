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
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "env.h"

static int bl_index_lut[LCD_MAX_DRV];
static struct aml_bl_drv_s *bl_driver[LCD_MAX_DRV];

struct aml_bl_drv_s *aml_bl_get_driver(int index)
{
	if (index >= LCD_MAX_DRV)
		return NULL;
	if (!bl_driver[index])
		return NULL;
	if (bl_driver[index]->config.method >= BL_CTRL_MAX)
		return NULL;

	return bl_driver[index];
}

static struct bl_config_s *bl_check_valid(struct aml_bl_drv_s *bdrv)
{
	struct bl_config_s *bconf;
	unsigned int bconf_flag = 1;
#ifdef CONFIG_AML_LCD_BL_EXTERN
	struct aml_bl_extern_driver_s *bl_ext;
#endif
#ifdef CONFIG_AML_LCD_BL_LDIM
	struct aml_ldim_driver_s *ldim_drv;
#endif

	if (!bdrv)
		return NULL;

	bconf = &bdrv->config;
	switch (bconf->method) {
	case BL_CTRL_PWM:
		if (!bconf->bl_pwm) {
			BLERR("no bl_pwm struct\n");
			bconf_flag = 0;
		}
		break;
	case BL_CTRL_PWM_COMBO:
		if (!bconf->bl_pwm_combo0) {
			BLERR("no bl_pwm_combo_0 struct\n");
			bconf_flag = 0;
		}
		if (!bconf->bl_pwm_combo1) {
			BLERR("no bl_pwm_combo_1 struct\n");
			bconf_flag = 0;
		}
		break;
	case BL_CTRL_GPIO:
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			bconf_flag = 0;
			break;
		}
		ldim_drv = aml_ldim_get_driver();
		if (!ldim_drv) {
			BLERR("no ldim driver\n");
			bconf_flag = 0;
		}
		break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	case BL_CTRL_EXTERN:
		bl_ext = aml_bl_extern_get_driver();
		if (!bl_ext) {
			BLERR("no bl_extern driver\n");
			bconf_flag = 0;
		}
		break;
#endif
	default:
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
			BLPR("invalid control_method: %d\n", bconf->method);
		bconf_flag = 0;
		break;
	}

	if (!bconf_flag)
		bconf = NULL;

	return bconf;
}

static void bl_pwm_pinmux_gpio_set(struct aml_bl_drv_s *bdrv, int pwm_index, int gpio_level)
{
	struct bl_config_s *bconf;
	struct bl_pwm_config_s *bl_pwm = NULL;
	int gpio;
	char *str;
	int i;

	bconf = bl_check_valid(bdrv);
	if (!bconf)
		return;

	switch (bconf->method) {
	case BL_CTRL_PWM:
		bl_pwm = bconf->bl_pwm;
		break;
	case BL_CTRL_PWM_COMBO:
		if (pwm_index == 0)
			bl_pwm = bconf->bl_pwm_combo0;
		else
			bl_pwm = bconf->bl_pwm_combo1;
		break;
	default:
		BLERR("%s: invalid method %d\n", __func__, bconf->method);
		break;
	}
	if (!bl_pwm)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		BLPR("%s: pwm_port=0x%x, pinmux_flag=%d\n",
			__func__, bl_pwm->pwm_port, bl_pwm->pinmux_flag);
	}
	if (bl_pwm->pinmux_flag > 0) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_clr_mask(bl_pwm->pinmux_clr[i][0], bl_pwm->pinmux_clr[i][1]);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
				BLPR("%s: port=0x%x, pinmux_clr=0x%x,0x%08x\n",
				     __func__, bl_pwm->pwm_port,
				     bl_pwm->pinmux_clr[i][0], bl_pwm->pinmux_clr[i][1]);
			}
			i++;
		}
		bl_pwm->pinmux_flag = 0;
	}
	/* set gpio */
	if (bl_pwm->pwm_gpio >= BL_GPIO_NUM_MAX) {
		gpio = LCD_GPIO_MAX;
	} else {
		str = bconf->gpio_name[bl_pwm->pwm_gpio];
		gpio = lcd_gpio_name_map_num(str);
	}
	if (gpio < LCD_GPIO_MAX)
		lcd_gpio_set(gpio, gpio_level);
}

static void bl_pwm_pinmux_gpio_clr(struct aml_bl_drv_s *bdrv, unsigned int pwm_index)
{
	struct bl_config_s *bconf;
	struct bl_pwm_config_s *bl_pwm = NULL;
	int i;

	bconf = bl_check_valid(bdrv);
	if (!bconf)
		return;

	switch (bconf->method) {
	case BL_CTRL_PWM:
		bl_pwm = bconf->bl_pwm;
		break;
	case BL_CTRL_PWM_COMBO:
		if (pwm_index == 0)
			bl_pwm = bconf->bl_pwm_combo0;
		else
			bl_pwm = bconf->bl_pwm_combo1;
		break;
	default:
		BLERR("%s: invalid method %d\n", __func__, bconf->method);
		break;
	}
	if (!bl_pwm)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		BLPR("%s: pwm_port=0x%x, pinmux_flag=%d\n",
			__func__, bl_pwm->pwm_port, bl_pwm->pinmux_flag);
	}
	if (bl_pwm->pinmux_flag > 0)
		return;

	/* set pinmux */
	i = 0;
	while (i < LCD_PINMUX_NUM) {
		if (bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
			break;
		lcd_pinmux_clr_mask(bl_pwm->pinmux_clr[i][0], bl_pwm->pinmux_clr[i][1]);
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			BLPR("%s: port=0x%x, pinmux_clr=0x%x,0x%08x\n",
			     __func__, bl_pwm->pwm_port,
			     bl_pwm->pinmux_clr[i][0], bl_pwm->pinmux_clr[i][1]);
		}
		i++;
	}
	i = 0;
	while (i < LCD_PINMUX_NUM) {
		if (bl_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
			break;
		lcd_pinmux_set_mask(bl_pwm->pinmux_set[i][0], bl_pwm->pinmux_set[i][1]);
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			BLPR("%s: port=0x%x, pinmux_set=0x%x,0x%08x\n",
			     __func__, bl_pwm->pwm_port,
			     bl_pwm->pinmux_set[i][0], bl_pwm->pinmux_set[i][1]);
		}
		i++;
	}
	bl_pwm->pinmux_flag = 1;
}

void bl_set_pwm_gpio_check(struct aml_bl_drv_s *bdrv, struct bl_pwm_config_s *bl_pwm)
{
	unsigned int pwm_index, gpio_level;

	pwm_index = bl_pwm->index;

	/* pwm duty 100% or 0% special control */
	if (bl_pwm->pwm_duty == 0 || bl_pwm->pwm_duty == bl_pwm->pwm_duty_range) {
		switch (bl_pwm->pwm_method) {
		case BL_PWM_POSITIVE:
			if (bl_pwm->pwm_duty == 0)
				gpio_level = 0;
			else
				gpio_level = 1;
			break;
		case BL_PWM_NEGATIVE:
			if (bl_pwm->pwm_duty == 0)
				gpio_level = 1;
			else
				gpio_level = 0;
			break;
		default:
			BLERR("%s: port=0x%x: invalid pwm_method %d\n",
				__func__, bl_pwm->pwm_port,
				bl_pwm->pwm_method);
			gpio_level = 0;
			break;
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			BLPR("%s: pwm port=0x%x, duty=%d, switch to gpio %d\n",
				__func__, bl_pwm->pwm_port,
				bl_pwm->pwm_duty, gpio_level);
		}
		bl_pwm_pinmux_gpio_set(bdrv, pwm_index, gpio_level);
	} else {
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			BLPR("%s: pwm_port=0x%x set as pwm\n",
				__func__, bl_pwm->pwm_port);
		}
		bl_pwm_pinmux_gpio_clr(bdrv, pwm_index);
	}
}

static void bl_pwm_pinmux_ctrl(struct aml_bl_drv_s *bdrv, int status)
{
	struct bl_config_s *bconf = &bdrv->config;
	int gpio;
	char *str;
	int i;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		BLPR("%s: %d\n", __func__, status);
	if (status) {
		/* set pinmux */
		switch (bconf->method) {
		case BL_CTRL_PWM:
			bl_set_pwm_gpio_check(bdrv, bconf->bl_pwm);
			break;
		case BL_CTRL_PWM_COMBO:
			bl_set_pwm_gpio_check(bdrv, bconf->bl_pwm_combo0);
			bl_set_pwm_gpio_check(bdrv, bconf->bl_pwm_combo1);
			break;
		default:
			break;
		}
	} else {
		switch (bconf->method) {
		case BL_CTRL_PWM:
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (bconf->bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				lcd_pinmux_clr_mask(bconf->bl_pwm->pinmux_clr[i][0],
						    bconf->bl_pwm->pinmux_clr[i][1]);
				if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
					BLPR("%s: port=0x%x, pinmux_clr=0x%x,0x%08x\n",
					     __func__, bconf->bl_pwm->pwm_port,
					     bconf->bl_pwm->pinmux_clr[i][0],
					     bconf->bl_pwm->pinmux_clr[i][1]);
				}
				i++;
			}
			bconf->bl_pwm->pinmux_flag = 0;

			if (bconf->bl_pwm->pwm_gpio >= BL_GPIO_NUM_MAX) {
				gpio = LCD_GPIO_MAX;
			} else {
				str = bconf->gpio_name[bconf->bl_pwm->pwm_gpio];
				gpio = lcd_gpio_name_map_num(str);
			}
			if (gpio < LCD_GPIO_MAX)
				lcd_gpio_set(gpio, bconf->bl_pwm->pwm_gpio_off);
			break;
		case BL_CTRL_PWM_COMBO:
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (bconf->bl_pwm_combo0->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				lcd_pinmux_clr_mask(bconf->bl_pwm_combo0->pinmux_clr[i][0],
						    bconf->bl_pwm_combo0->pinmux_clr[i][1]);
				if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
					BLPR("%s: port=0x%x, pinmux_clr=0x%x,0x%08x\n",
					     __func__, bconf->bl_pwm_combo0->pwm_port,
					     bconf->bl_pwm_combo0->pinmux_clr[i][0],
					     bconf->bl_pwm_combo0->pinmux_clr[i][1]);
				}
				i++;
			}
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (bconf->bl_pwm_combo1->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				lcd_pinmux_clr_mask(bconf->bl_pwm_combo1->pinmux_clr[i][0],
						    bconf->bl_pwm_combo1->pinmux_clr[i][1]);
				if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
					BLPR("%s: port=0x%x, pinmux_clr=0x%x,0x%08x\n",
					     __func__, bconf->bl_pwm_combo1->pwm_port,
					     bconf->bl_pwm_combo1->pinmux_clr[i][0],
					     bconf->bl_pwm_combo1->pinmux_clr[i][1]);
				}
				i++;
			}
			bconf->bl_pwm_combo0->pinmux_flag = 0;
			bconf->bl_pwm_combo1->pinmux_flag = 0;

			if (bconf->bl_pwm_combo0->pwm_gpio >= BL_GPIO_NUM_MAX) {
				gpio = LCD_GPIO_MAX;
			} else {
				str = bconf->gpio_name[bconf->bl_pwm_combo0->pwm_gpio];
				gpio = lcd_gpio_name_map_num(str);
			}
			if (gpio < LCD_GPIO_MAX)
				lcd_gpio_set(gpio, bconf->bl_pwm_combo0->pwm_gpio_off);
			if (bconf->bl_pwm_combo1->pwm_gpio >= BL_GPIO_NUM_MAX) {
				gpio = LCD_GPIO_MAX;
			} else {
				str = bconf->gpio_name[bconf->bl_pwm_combo1->pwm_gpio];
				gpio = lcd_gpio_name_map_num(str);
			}
			if (gpio < LCD_GPIO_MAX)
				lcd_gpio_set(gpio, bconf->bl_pwm_combo1->pwm_gpio_off);
			break;
		default:
			break;
		}
	}
}

static void bl_pwm_config_update(struct aml_bl_drv_s *bdrv)
{
#ifdef CONFIG_AML_LCD_BL_LDIM
	struct aml_ldim_driver_s *ldim_drv;
#endif

	switch (bdrv->config.method) {
	case BL_CTRL_PWM:
		bl_pwm_config_init(bdrv->config.bl_pwm);
		break;
	case BL_CTRL_PWM_COMBO:
		bl_pwm_config_init(bdrv->config.bl_pwm_combo0);
		bl_pwm_config_init(bdrv->config.bl_pwm_combo1);
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}
		ldim_drv = aml_ldim_get_driver();
		if (!ldim_drv || !ldim_drv->dev_drv) {
			BLERR("ldim_drv or dev_drv is null\n");
			break;
		}
		if (ldim_drv->dev_drv->ldim_pwm_config.pwm_port >= BL_PWM_MAX)
			break;
		bl_pwm_config_init(&ldim_drv->dev_drv->ldim_pwm_config);
		if (ldim_drv->dev_drv->analog_pwm_config.pwm_port < BL_PWM_VS)
			bl_pwm_config_init(&ldim_drv->dev_drv->analog_pwm_config);
		break;
#endif
	default:
		break;
	}
}

static unsigned int bl_level_mapping(struct bl_config_s *bconf, unsigned int level)
{
	unsigned int mid = bconf->level_mid;
	unsigned int mid_map = bconf->level_mid_mapping;
	unsigned int max = bconf->level_max;
	unsigned int min = bconf->level_min;

	if (mid == mid_map)
		return level;

	level = level > max ? max : level;
	if (level >= mid && level <= max)
		level = (((level - mid) * (max - mid_map)) / (max - mid)) + mid_map;
	else if (level >= min && level < mid)
		level = (((level - min) * (mid_map - min)) / (mid - min)) + min;
	else
		level = min;

	return level;
}

static void bl_set_level(struct aml_bl_drv_s *bdrv, unsigned int level)
{
	struct bl_config_s *bconf;
	struct bl_pwm_config_s *pwm0, *pwm1;
#ifdef CONFIG_AML_LCD_BL_EXTERN
	struct aml_bl_extern_driver_s *bl_ext;
#endif
#ifdef CONFIG_AML_LCD_BL_LDIM
	struct aml_ldim_driver_s *ldim_drv;
#endif

	bconf = bl_check_valid(bdrv);
	if (!bconf)
		return;

	BLPR("set level: %u, last level: %u\n", level, bdrv->level);
	/* level range check */
	level = bl_level_mapping(bconf, level);
	bdrv->level = level;

	switch (bconf->method) {
	case BL_CTRL_GPIO:
		break;
	case BL_CTRL_PWM:
		bl_pwm_set_level(bdrv, bconf->bl_pwm, level);
		break;
	case BL_CTRL_PWM_COMBO:
		pwm0 = bconf->bl_pwm_combo0;
		pwm1 = bconf->bl_pwm_combo1;

		if (level >= pwm0->bl_level_max) {
			bl_pwm_set_level(bdrv, pwm0, pwm0->bl_level_max);
		} else if ((level > pwm0->bl_level_min) &&
			(level < pwm0->bl_level_max)) {
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				BLPR("pwm0 region, level=%u\n", level);
			bl_pwm_set_level(bdrv, pwm0, level);
		} else {
			bl_pwm_set_level(bdrv, pwm0, pwm0->bl_level_min);
		}

		if (level >= pwm1->bl_level_max) {
			bl_pwm_set_level(bdrv, pwm1, pwm1->bl_level_max);
		} else if ((level > pwm1->bl_level_min) &&
			(level < pwm1->bl_level_max)) {
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				BLPR("pwm1 region, level=%u\n", level);
			bl_pwm_set_level(bdrv, pwm1, level);
		} else {
			bl_pwm_set_level(bdrv, pwm1, pwm1->bl_level_min);
		}
		break;
#ifdef CONFIG_AML_LCD_BL_LDIM
	case BL_CTRL_LOCAL_DIMMING:
		if (bdrv->index > 0) {
			BLERR("no ldim driver\n");
			break;
		}
		ldim_drv = aml_ldim_get_driver();
		if (ldim_drv->set_level)
			ldim_drv->set_level(ldim_drv, level);
		else
			BLERR("ldim set_level is null\n");
		break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
	case BL_CTRL_EXTERN:
		bl_ext = aml_bl_extern_get_driver();
		if (bl_ext->set_level)
			bl_ext->set_level(level);
		else
			BLERR("bl_extern set_level is null\n");
		break;
#endif
	default:
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
			BLERR("wrong backlight control method\n");
		break;
	}
}

static void bl_power_en_ctrl(struct bl_config_s *bconf, int status)
{
	int gpio;
	char *str;

	if (bconf->en_gpio >= BL_GPIO_NUM_MAX) {
		gpio = LCD_GPIO_MAX;
	} else {
		str = bconf->gpio_name[bconf->en_gpio];
		gpio = lcd_gpio_name_map_num(str);
	}
	if (status) {
		if (gpio < LCD_GPIO_MAX)
			lcd_gpio_set(gpio, bconf->en_gpio_on);
	} else {
		if (gpio < LCD_GPIO_MAX)
			lcd_gpio_set(gpio, bconf->en_gpio_off);
	}
}

static void bl_pwm_en_ctrl(struct bl_config_s *bconf, int status)
{
	switch (bconf->method) {
	case BL_CTRL_PWM:
		bl_pwm_en(bconf->bl_pwm, status);
		break;
	case BL_CTRL_PWM_COMBO:
		bl_pwm_en(bconf->bl_pwm_combo0, status);
		bl_pwm_en(bconf->bl_pwm_combo1, status);
		break;
	default:
		break;
	}
}

static void bl_power_ctrl(struct aml_bl_drv_s *bdrv, int status)
{
	int gpio, value;
	struct bl_config_s *bconf;
#ifdef CONFIG_AML_LCD_BL_EXTERN
	struct aml_bl_extern_driver_s *bl_ext;
#endif
#ifdef CONFIG_AML_LCD_BL_LDIM
	struct aml_ldim_driver_s *ldim_drv;
#endif

	bconf = bl_check_valid(bdrv);
	if (!bconf)
		return;

	gpio = bconf->en_gpio;
	value = status ? bconf->en_gpio_on : bconf->en_gpio_off;
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		BLPR("status=%d gpio=%d value=%d\n", status, gpio, value);

	if (status) {
		/* bl_off_policy */
		if (bdrv->bl_off_policy != BL_OFF_POLICY_NONE) {
			BLPR("bl_off_policy=%d for bl_off\n", bdrv->bl_off_policy);
			return;
		}

		bdrv->state = 1;
		bl_pwm_en_ctrl(bconf, 1);
		/* check if factory test */
		if (bdrv->factory_bl_on_delay >= 0) {
			BLPR("%s: factory test power_on_delay!\n", __func__);
			if (bdrv->factory_bl_on_delay > 0)
				mdelay(bdrv->factory_bl_on_delay);
		} else {
			if (bconf->power_on_delay > 0)
				mdelay(bconf->power_on_delay);
		}

		switch (bconf->method) {
		case BL_CTRL_GPIO:
			bl_power_en_ctrl(bconf, 1);
			break;
		case BL_CTRL_PWM:
			if (bconf->en_sequence_reverse) {
				/* step 1: power on enable */
				bl_power_en_ctrl(bconf, 1);
				if (bconf->pwm_on_delay > 0)
					mdelay(bconf->pwm_on_delay);
				/* step 2: power on pwm */
				bl_pwm_pinmux_ctrl(bdrv, 1);
			} else {
				/* step 1: power on pwm */
				bl_pwm_pinmux_ctrl(bdrv, 1);
				if (bconf->pwm_on_delay > 0)
					mdelay(bconf->pwm_on_delay);
				/* step 2: power on enable */
				bl_power_en_ctrl(bconf, 1);
			}
			break;
		case BL_CTRL_PWM_COMBO:
			if (bconf->en_sequence_reverse) {
				/* step 1: power on enable */
				bl_power_en_ctrl(bconf, 1);
				if (bconf->pwm_on_delay > 0)
					mdelay(bconf->pwm_on_delay);
				/* step 2: power on pwm_combo */
				bl_pwm_pinmux_ctrl(bdrv, 1);
			} else {
				/* step 1: power on pwm_combo */
				bl_pwm_pinmux_ctrl(bdrv, 1);
				if (bconf->pwm_on_delay > 0)
					mdelay(bconf->pwm_on_delay);
				/* step 2: power on enable */
				bl_power_en_ctrl(bconf, 1);
			}
			break;
#ifdef CONFIG_AML_LCD_BL_LDIM
		case BL_CTRL_LOCAL_DIMMING:
			if (bdrv->index > 0) {
				BLERR("no ldim driver\n");
				break;
			}
			ldim_drv = aml_ldim_get_driver();
			if (bconf->en_sequence_reverse) {
				/* step 1: power on enable */
				bl_power_en_ctrl(bconf, 1);
				/* step 2: power on ldim */
				if (ldim_drv->power_on)
					ldim_drv->power_on(ldim_drv);
				else
					BLERR("ldim power on is null\n");
			} else {
				/* step 1: power on ldim */
				if (ldim_drv->power_on)
					ldim_drv->power_on(ldim_drv);
				else
					BLERR("ldim power on is null\n");
				/* step 2: power on enable */
				bl_power_en_ctrl(bconf, 1);
			}
			break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
		case BL_CTRL_EXTERN:
			bl_ext = aml_bl_extern_get_driver();
			if (bconf->en_sequence_reverse) {
				/* step 1: power on enable */
				bl_power_en_ctrl(bconf, 1);
				/* step 2: power on bl_extern */
				if (bl_ext->power_on)
					bl_ext->power_on();
				else
					BLERR("bl_extern power on is null\n");
			} else {
				/* step 1: power on bl_extern */
				if (bl_ext->power_on)
					bl_ext->power_on();
				else
					BLERR("bl_extern power on is null\n");
				/* step 2: power on enable */
				bl_power_en_ctrl(bconf, 1);
			}
			break;
#endif
		default:
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				BLERR("wrong backlight control method\n");
			break;
		}
	} else {
		bdrv->state = 0;
		switch (bconf->method) {
		case BL_CTRL_GPIO:
			bl_power_en_ctrl(bconf, 0);
			break;
		case BL_CTRL_PWM:
			if (bconf->en_sequence_reverse == 1) {
				/* step 1: power off pwm */
				bl_pwm_pinmux_ctrl(bdrv, 0);
				bl_pwm_en(bconf->bl_pwm, 0);
				if (bconf->pwm_off_delay > 0)
					mdelay(bconf->pwm_off_delay);
				/* step 2: power off enable */
				bl_power_en_ctrl(bconf, 0);
			} else {
				/* step 1: power off enable */
				bl_power_en_ctrl(bconf, 0);
				/* step 2: power off pwm */
				if (bconf->pwm_off_delay > 0)
					mdelay(bconf->pwm_off_delay);
				bl_pwm_pinmux_ctrl(bdrv, 0);
				bl_pwm_en(bconf->bl_pwm, 0);
			}
			break;
		case BL_CTRL_PWM_COMBO:
			if (bconf->en_sequence_reverse == 1) {
				/* step 1: power off pwm_combo */
				bl_pwm_pinmux_ctrl(bdrv, 0);
				bl_pwm_en(bconf->bl_pwm_combo0, 0);
				bl_pwm_en(bconf->bl_pwm_combo1, 0);
				if (bconf->pwm_off_delay > 0)
					mdelay(bconf->pwm_off_delay);
				/* step 2: power off enable */
				bl_power_en_ctrl(bconf, 0);
			} else {
				/* step 1: power off enable */
				bl_power_en_ctrl(bconf, 0);
				/* step 2: power off pwm_combo */
				if (bconf->pwm_off_delay > 0)
					mdelay(bconf->pwm_off_delay);
				bl_pwm_pinmux_ctrl(bdrv, 0);
				bl_pwm_en(bconf->bl_pwm_combo0, 0);
				bl_pwm_en(bconf->bl_pwm_combo1, 0);
			}
			break;
#ifdef CONFIG_AML_LCD_BL_LDIM
		case BL_CTRL_LOCAL_DIMMING:
			if (bdrv->index > 0) {
				BLERR("no ldim driver\n");
				break;
			}
			ldim_drv = aml_ldim_get_driver();
			if (bconf->en_sequence_reverse == 1) {
				/* step 1: power off ldim */
				if (ldim_drv->power_off)
					ldim_drv->power_off(ldim_drv);
				else
					BLERR("ldim power off is null\n");
				/* step 2: power off enable */
				bl_power_en_ctrl(bconf, 0);
			} else {
				/* step 1: power off enable */
				bl_power_en_ctrl(bconf, 0);
				/* step 2: power off ldim */
				if (ldim_drv->power_off)
					ldim_drv->power_off(ldim_drv);
				else
					BLERR("ldim power off is null\n");
			}
			break;
#endif
#ifdef CONFIG_AML_LCD_BL_EXTERN
		case BL_CTRL_EXTERN:
			bl_ext = aml_bl_extern_get_driver();
			if (bconf->en_sequence_reverse == 1) {
				/* step 1: power off bl_extern */
				if (bl_ext->power_off)
					bl_ext->power_off();
				else
					BLERR("bl_extern: power off is null\n");
				/* step 2: power off enable */
				bl_power_en_ctrl(bconf, 0);
			} else {
				/* step 1: power off enable */
				bl_power_en_ctrl(bconf, 0);
				/* step 2: power off bl_extern */
				if (bl_ext->power_off)
					bl_ext->power_off();
				else
					BLERR("bl_extern: power off is null\n");
			}
			break;
#endif
		default:
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				BLERR("wrong backlight control method\n");
			break;
		}
		if (bconf->power_off_delay > 0)
			mdelay(bconf->power_off_delay);
	}
	BLPR("%s: %d\n", __func__, status);
}

static void bl_power_init_off(struct aml_bl_drv_s *bdrv)
{
	struct bl_config_s *bconf;

	bconf = bl_check_valid(bdrv);
	if (!bconf)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		BLPR("[%d]: init_off: gpio=%d value=%d\n",
		      bdrv->index,
		      bconf->en_gpio, bconf->en_gpio_off);
	}

	bdrv->state = 0;
	switch (bconf->method) {
	case BL_CTRL_PWM:
	case BL_CTRL_PWM_COMBO:
		bl_power_en_ctrl(bconf, 0);
		bl_pwm_pinmux_ctrl(bdrv, 0);
		break;
	default:
		bl_power_en_ctrl(bconf, 0);
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		BLPR("%s finish\n", __func__);
}

static struct aml_bl_drv_s *bl_driver_add_single(unsigned char index)
{
	struct aml_bl_drv_s *bdrv = bl_driver[index];

	if (bl_index_lut[index] >= BL_INDEX_INVALID)
		return NULL;

	if (!bdrv) {
		bdrv = (struct aml_bl_drv_s *)malloc(sizeof(struct aml_bl_drv_s));
		if (!bdrv) {
			BLERR("%s: Not enough memory\n", __func__);
			return NULL;
		}
	}
	bl_driver[index] = bdrv;
	memset(bdrv, 0, sizeof(struct aml_bl_drv_s));
	bdrv->index = index;
	bdrv->data = aml_lcd_get_data();

	/* default config */
	bdrv->config.index = bl_index_lut[index];
	bdrv->config.method = BL_CTRL_MAX;
	bdrv->config.en_gpio = 0xff;
	bdrv->config.extern_index = 0xff;
	bdrv->factory_bl_on_delay = -1;

	return bdrv;
}

static void bl_driver_remove_single(unsigned char index)
{
	free(bl_driver[index]);
	bl_driver[index] = NULL;
}

void aml_bl_probe_single(unsigned char index, int load_id)
{
	struct aml_bl_drv_s *bdrv = bl_driver_add_single(index);
	int ret;

	if (!bdrv)
		return;

	ret = aml_bl_load_config(bdrv, lcd_get_dt_addr(), load_id);
	if (ret) {
		bl_driver_remove_single(index);
		return;
	}
	bl_power_init_off(bdrv);
}

void aml_bl_remove_all(void)
{
	int i;

	for (i = 0; i < LCD_MAX_DRV; i++)
		bl_driver_remove_single(i);
}

int aml_bl_index_add(int drv_index, int conf_index)
{
	if (drv_index >= LCD_MAX_DRV) {
		BLERR("%s: invalid drv_index: %d\n", __func__, drv_index);
		return -1;
	}

	bl_index_lut[drv_index] = conf_index;
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		BLPR("%s: drv_index %d, config index: %d\n",
			__func__, drv_index, conf_index);
	}
	return 0;
}

int aml_bl_index_remove(int drv_index)
{
	if (drv_index >= LCD_MAX_DRV) {
		BLERR("%s: invalid drv_index: %d\n", __func__, drv_index);
		return -1;
	}

	bl_index_lut[drv_index] = BL_INDEX_INVALID;
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		BLPR("%s: drv_index %d\n", __func__, drv_index);

	return 0;
}

int aml_bl_init(void)
{
	int i;
	struct aml_lcd_data_s *lcd_data = aml_lcd_get_data();

	aml_bl_pwm_reg_config_init(lcd_data);

	for (i = 0; i < LCD_MAX_DRV; i++) {
		bl_driver_remove_single(i);
		bl_index_lut[i] = BL_INDEX_INVALID;
	}

	return 0;
}

void aml_bl_driver_enable(int index)
{
	struct aml_bl_drv_s *bdrv;

	bdrv = aml_bl_get_driver(index);
	if (!bdrv)
		return;

	if (bdrv->state) {
		BLPR("already enabled\n");
		return;
	}
	bl_pwm_config_update(bdrv);
	bl_set_level(bdrv, bdrv->config.level_default);
	bl_power_ctrl(bdrv, 1);
}

void aml_bl_driver_disable(int index)
{
	struct aml_bl_drv_s *bdrv;

	bdrv = aml_bl_get_driver(index);
	if (!bdrv)
		return;

	if (!bdrv->state) {
		BLPR("already disabled\n");
		return;
	}

	bl_power_ctrl(bdrv, 0);
}

void aml_bl_set_level(int index, unsigned int level)
{
	struct aml_bl_drv_s *bdrv;

	bdrv = aml_bl_get_driver(index);
	if (!bdrv)
		return;

	bl_set_level(bdrv, level);
}

unsigned int aml_bl_get_level(int index)
{
	struct aml_bl_drv_s *bdrv;

	bdrv = aml_bl_get_driver(index);
	if (!bdrv)
		return 0;

	return bdrv->level;
}

void aml_bl_config_print(int index)
{
	struct aml_bl_drv_s *bdrv;

	bdrv = aml_bl_get_driver(index);
	if (!bdrv)
		return;

	bl_config_print(bdrv);
}
