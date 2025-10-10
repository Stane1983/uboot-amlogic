// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <common.h>
#include <command.h>
#include <amlogic/aml_model.h>
#include <asm/gpio.h>
#include <dm/uclass.h>
#include <linux/err.h>
#include <dm/device.h>

static int get_gpio_status(int gpio_id)
{
	struct udevice *dev;
	int ret = -1;
	for (ret = uclass_first_device_check(UCLASS_GPIO, &dev);
		dev;
		ret = uclass_next_device_check(&dev)) {
		int num_bits;
		//printf("GPIO device %s probe error %i\n", dev->name, ret);
		if (ret) {
			continue;
		}
		gpio_get_bank_info(dev, &num_bits);
		//printf("GPIO device %s num_bits = %d\n", dev->name, num_bits);
		if (!num_bits) {
			continue;
		}
		struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
		//printf("GPIO device base %d count = %d\n", uc_priv->gpio_base, uc_priv->gpio_count);
		if (gpio_id < uc_priv->gpio_base ||
		    gpio_id >= uc_priv->gpio_base + uc_priv->gpio_count) {
			continue;
		}
		int offset = gpio_id - uc_priv->gpio_base;
		ret = gpio_get_function(dev, offset, NULL);
		break;
	}
	return ret;
}

static int do_afm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *str_cmd, *str_value = NULL;
	if (argc == 2 && !strcmp(argv[1], "print"))
	{
		char *serialno = env_get("usid");
		char *mac = env_get("mac");
		char *console = env_get("console");
		char *silent = env_get("silent");
		char *powermode = env_get("powermode");
		char *otg_device = env_get("otg_device");
		char *selinux = env_get("EnableSelinux");
		char model_name[64] = {0}, model1_name[64] = {0}, model2_name[64] = {0};
		char modelt_name[150] = {0};

		handle_model_get("0", model_name, 64);
		handle_model_get("1", model1_name, 64);
		handle_model_get("2", model2_name, 64);
		if (strlen(model_name) != 0)
		{
			strcat(modelt_name, model_name);
		}
		if (strlen(model1_name) != 0) {
			strcat(modelt_name, "|");
			strcat(modelt_name, model1_name);
		}
		if (strlen(model2_name) !=0 ) {
			strcat(modelt_name, "|");
			strcat(modelt_name, model2_name);
		}
		//printf("current %s: %s\n", str, model_name);
		printf("\n"
			"model_name :     %s\n"
			"serialno :       %s\n"
			"mac :            %s\n"
			"console :        %s\n"
			"silent :         %s\n"
			"powermode :      %s\n"
			"otg_device :     %s\n"
			"selinux :        %s\n"
			, strlen(modelt_name) != 0 ? modelt_name : ""
			, serialno != NULL? serialno : ""
			, mac != NULL? mac : ""
			, console != NULL && !strcmp(console, "ttyS0,115200") ? "on" : "off"
			, silent != NULL? (!strcmp(silent, "0")? "on" : "off") : "on"
			, powermode != NULL? powermode : ""
			, otg_device != NULL? otg_device : ""
			, selinux != NULL? selinux : "");
		return 0;
	}
	if (argc == 4) {
		if (!strcmp(argv[1], "print")) {
			int gpio_id = dectoul(argv[2], NULL);
			char* printContent = argv[3];
			if (gpio_id > 0
				&&  (strcmp(printContent, "dir") == 0 || strcmp(printContent, "output") == 0)) {
				/* grab the pin before we tweak it */
				int ret = gpio_request(gpio_id, "cmd_gpio");
				if (ret && ret != -EBUSY) {
					printf("gpio: requesting pin %u failed\n", gpio_id);
					return 0;
				}
				if (strcmp(printContent, "dir") == 0) {
					printf("gpio %d direction state :     %d\n", gpio_id, get_gpio_status(gpio_id));
				} else {
					printf("gpio %d output vlaue :     %d\n", gpio_id, gpio_get_value(gpio_id));
				}
				if (ret != -EBUSY)
					gpio_free(gpio_id);
				return 0;
			} else {
				printf("input invalid \n");
				return CMD_RET_USAGE;
			}
		}
		int gpio_id = dectoul(argv[1], NULL);
		char* setContent = argv[2];
		char* setValue = argv[3];
		int value = -1;
		if (gpio_id > 0
			&&  (strcmp(setContent, "dir") == 0 || strcmp(setContent, "output") == 0)) {
			/* grab the pin before we tweak it */
			int ret = gpio_request(gpio_id, "cmd_gpio");
			if (ret && ret != -EBUSY) {
				printf("gpio: requesting pin %u failed\n", gpio_id);
				return 0;
			}
			if (strcmp(setContent, "dir") == 0) {
				if (!strcmp(setValue, "0")) {
					gpio_direction_input(gpio_id);
				} else if (!strcmp(setValue, "1")) {
					value = gpio_get_value(gpio_id);
					gpio_direction_output(gpio_id, value);
				} else {
					printf("dir value invalid \n");
				}
			} else {
				if (!strcmp(setValue, "0") || !strcmp(setValue, "1")) {
					value = dectoul(setValue, NULL);
					gpio_set_value(gpio_id, value);
				} else {
					printf("output value invalid \n");
				}
			}
			/*printf("gpio: gpio %u value is ", gpio_id);
			if (IS_ERR_VALUE(value)) {
				printf("unknown (ret=%d)\n", value);
			} else {
				printf("%d\n", value);
			}*/
			if (ret != -EBUSY)
				gpio_free(gpio_id);
			return 0;
		} else {
			printf("input invalid \n");
			return CMD_RET_USAGE;
		}
	}
	if (argc != 3)
		return CMD_RET_USAGE;
	str_cmd = argv[1];
	/* Strip off leading 'afm' command argument */
	argc -= 2;
	argv += 2;
	//if (argc > 0)
		str_value = *argv;
	if (!strcmp(str_cmd, "otg_device")) {
		if (!strcmp(str_value, "1")) {
			run_command("setenv otg_device 1", 0);
		} else if (!strcmp(str_value, "0")) {
			run_command("setenv otg_device 0", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
		return 0;
	} else if (!strcmp(str_cmd, "serialno")) {
		char result[50] = "keyman write usid str ";//size 21
		if (strlen(str_value) > 17) {//str_value size 17
			printf("invalid no\n");
			return CMD_RET_USAGE;
		}
		strcat(result, str_value);
		run_command(result, 0);
		run_command("keyman read usid $loadaddr str; printenv usid", 0);
		return 0;
	} else if (!strcmp(str_cmd, "mac")) {
		char result[50] = "keyman write mac str ";//size 21

		if (strlen(str_value) != 17) {//str_value size 17
			printf("invalid address\n");
			return CMD_RET_USAGE;
		}
		strcat(result, str_value);
		run_command(result, 0);
		run_command("keyman read mac $loadaddr str; printenv mac", 0);
		return 0;
	} else if (!strcmp(str_cmd, "silent")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv silent 0", 0);
		} else if (!strcmp(str_value, "off")) {
			run_command("setenv silent 1", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "powermode")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv powermode on", 0);
		} else if (!strcmp(str_value, "standby")) {
			run_command("setenv powermode standby", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "selinux")) {
		if (!strcmp(str_value, "enforcing")) {
			run_command("setenv EnableSelinux enforcing", 0);
		} else if (!strcmp(str_value, "permissive")) {
			run_command("setenv EnableSelinux permissive", 0);
		} else if (!strcmp(str_value, "disabled")) {
			run_command("setenv EnableSelinux disabled", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "model_name")) {
		handle_model_set("model_name", str_value);
	} else if (!strcmp(str_cmd, "model1_name")) {
		handle_model_set("model1_name", str_value);
	} else if (!strcmp(str_cmd, "model2_name")) {
		handle_model_set("model2_name", str_value);
	} else if (!strcmp(str_cmd, "console")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv console ttyS0,115200", 0);
			run_command("setenv loglevel 7", 0);
		} else if (!strcmp(str_value, "off")) {
			run_command("setenv console off", 0);
			run_command("setenv loglevel 0", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else {
		printf("invalid value\n");
		return CMD_RET_USAGE;
	}
	return 0;
}

U_BOOT_CMD(afm, CONFIG_SYS_MAXARGS, 0, do_afm,
	"Factory related command/usage",
	"\n"
	"afm model_name <ModelName> - modify screen parameters\n"
	"afm serialno <serial no> - modify serial no\n"
	"afm mac <mac address> - modify mac address\n"
	"afm console <on|off> - modify console state\n"
	"afm silent <on|off> - modify the print state of uboot bl33 serial port\n"
	"afm powermode <on|standby> - modify power on mode\n"
	"afm otg_device <1|0> - modify the USB host/device state\n"
	"afm selinux <0|1|2(permissive|enforcing|disabled)> - modify the selinux mode\n"
	"afm gpio_id <dir|output> <0|1> - modify the gpio direction state/output vlaue\n"
	"afm print gpio_id <dir|output> - print the gpio direction state/output vlaue\n"
	"afm print - Print the values of all parameters\n"
);
