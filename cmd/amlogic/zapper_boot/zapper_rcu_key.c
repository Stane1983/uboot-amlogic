//#include <stdio.h>
#include <asm/io.h>
#include <common.h>
#include <command.h>
#include <vsprintf.h>
#include <amlogic/zapper_boot.h>

#define RCU_KEY_DETECT_CMD	"irkey"
#define RCU_KEY_VALUE_ENV	"irkey_value"
#define RCU_KEY_DETECT_TIMEOUT 3000000	/* us */

#define PRJ_TAG "ZAPPER"
#define RCU_TAG "<RCU_KEY>"
#define PRINT_TAG PRJ_TAG":"RCU_TAG

/**************************** KEY VALUE DEFINE ******************************/
#ifdef CONFIG_BG20AB_S805C1
#define KEY_VALUE_BACK 	0x1c00083
#define KEY_VALUE_INFO 	0x1c000cb
#define KEY_VALUE_0 	0x1c00000
#define KEY_VALUE_1 	0x1c00001
#define KEY_VALUE_2 	0x1c00002
#define KEY_VALUE_4 	0x1c00004
#define KEY_VALUE_5 	0x1c00005
#define KEY_VALUE_6 	0x1c00006
#define KEY_VALUE_8 	0x1c00008
#define KEY_VALUE_9 	0x1c00009
#else
/* AML RC */
#define KEY_VALUE_BACK 	0xbc43fe01
#define KEY_VALUE_INFO 	0xa659fe01
#define KEY_VALUE_0 	0xf50afe01
#define KEY_VALUE_1 	0xfe01fe01
#define KEY_VALUE_2 	0xfd02fe01
#define KEY_VALUE_4 	0xfb04fe01
#define KEY_VALUE_5 	0xfa05fe01
#define KEY_VALUE_6 	0xf906fe01
#define KEY_VALUE_8 	0xf708fe01
#define KEY_VALUE_9 	0xf609fe01
#endif
/**************************** KEY VALUE DEFINE ******************************/

#define KEY_VALUE_TOTAL_NUM 10	/* count for KEY_VALUE_BACK~KEY_VALUE_9 */

static unsigned char g_rcu_combinatoion_type = RCU_COMBINATION_MAX;

static int check_and_output_key_value(unsigned int *key_value_list, unsigned int cnt, unsigned int *output_key_value)
{
	int i;
	unsigned long key_value;
	char *tmp_str = NULL;

	tmp_str = env_get(RCU_KEY_VALUE_ENV);
	if (tmp_str == NULL) {
		return -1;
	}
	//sscanf(tmp_str, "0x%x", &key_value);
	strict_strtoul(tmp_str, 16, &key_value);

	for (i = 0; i < cnt; i++) {
		printf("%s keyvalue: 0x%lx, key-list: 0x%x\n", PRINT_TAG, key_value, key_value_list[i]);
		if (key_value == key_value_list[i]) {
			if (output_key_value != NULL) {
				/* copy the key_value to output_key_value */
				*output_key_value = key_value;
			}
			return 0;
		}
	}

	return -1;
}

static void get_and_save_the_combination_type(unsigned int *key_press_list, unsigned int cnt)
{
	if (cnt < 4) {
		printf("%s invalid rcu combination key \n", PRINT_TAG);
		return;
	}
	if (cnt == 4) {
		if ((key_press_list[0] == KEY_VALUE_1) && (key_press_list[1] == KEY_VALUE_5) &&
		(key_press_list[2] == KEY_VALUE_9) && (key_press_list[3] == KEY_VALUE_0)) {
			g_rcu_combinatoion_type = RCU_COMBINATION_FACTORY_RESET;
			goto exit;
		}
	}
	if (cnt == 6) {
		if ((key_press_list[0] == KEY_VALUE_BACK) && (key_press_list[1] == KEY_VALUE_2) &&
			(key_press_list[2] == KEY_VALUE_4) && (key_press_list[3] == KEY_VALUE_6) &&
			(key_press_list[4] == KEY_VALUE_5) && (key_press_list[5] == KEY_VALUE_INFO)) {
			g_rcu_combinatoion_type = RCU_COMBINATION_ADVANCED_SETUP_SCREEN;
			goto exit;
		}
	}
	if ((key_press_list[0] == KEY_VALUE_BACK) && (key_press_list[1] == KEY_VALUE_2) &&
		(key_press_list[2] == KEY_VALUE_4) && (key_press_list[3] == KEY_VALUE_6) &&
		(key_press_list[4] == KEY_VALUE_5)) {
		g_rcu_combinatoion_type = RCU_COMBINATION_ADVANCED_TUNING_CODE_SCREEN;
		goto exit;
	}

	if ((key_press_list[0] == KEY_VALUE_BACK) && (key_press_list[1] == KEY_VALUE_1) &&
		(key_press_list[2] == KEY_VALUE_5) && (key_press_list[3] == KEY_VALUE_8) &&
		(key_press_list[4] == KEY_VALUE_5)) {
		g_rcu_combinatoion_type = RCU_COMBINATION_USB_UPGRADE;
		goto exit;
	}

	if ((key_press_list[0] == KEY_VALUE_BACK) && (key_press_list[1] == KEY_VALUE_2) &&
		(key_press_list[2] == KEY_VALUE_4) && (key_press_list[3] == KEY_VALUE_8) &&
		(key_press_list[4] == KEY_VALUE_5)) {
		g_rcu_combinatoion_type = RCU_COMBINATION_MANUAL_FORCED_DOWNLOAD;
		goto exit;
	}

	if ((key_press_list[0] == KEY_VALUE_BACK) && (key_press_list[1] == KEY_VALUE_1) &&
		(key_press_list[2] == KEY_VALUE_5) && (key_press_list[3] == KEY_VALUE_9) &&
		(key_press_list[4] == KEY_VALUE_0)) {
		g_rcu_combinatoion_type = RCU_COMBINATION_FACTORY_RESET;
		goto exit;
	}
exit:
	printf("%s g_rcu_combinatoion_type = %d \n", PRINT_TAG, g_rcu_combinatoion_type);
	return;
}

/*
 * Advanced Tuning Code Screen	Back -> 2 -> 4 -> 6 -> 5
 * Advanced Setup Screen		Back -> 2 -> 4 -> 6 -> 5 -> i
 * USB Upgrade					Back -> 1 -> 5 -> 8 -> 5
 * Manual Forced Download		Back -> 2 -> 4 -> 8 -> 5
 * Factory Reset				 1   -> 5 -> 9 -> 0
 */
static int rcu_combination_key_detect(void)
{
	char cmd_str[50];
	unsigned int key_press_list[KEY_VALUE_TOTAL_NUM] = { 0 };
	unsigned int key_value_list[KEY_VALUE_TOTAL_NUM] = { 0 };
	unsigned int key_value_cnt = 0;
	unsigned int valid_cnt = 0;

	/* key-1 */
	{
		/* wait press 'BACK' or '1' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_BACK, KEY_VALUE_1);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the BACK button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_BACK;
		key_value_list[1] = KEY_VALUE_1;
		key_value_cnt = 2;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the 'BACK' or '1' key\n", PRINT_TAG);
			return ZAPPER_ERROR;
		}
		valid_cnt++;
	}

	/* key-2 */
	{
		/* wait press '1' or '2' or '5' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x 0x%x 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_1, KEY_VALUE_2, KEY_VALUE_5);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the '1' or '2' or '5' button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_1;
		key_value_list[1] = KEY_VALUE_2;
		key_value_list[2] = KEY_VALUE_5;
		key_value_cnt = 3;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the '1' or '2' or '5' key\n", PRINT_TAG);
			return ZAPPER_ERROR;
		}
		valid_cnt++;
	}

	/* key-3 */
	{
		/* wait press '4' or '5' or '9' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x 0x%x 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_4, KEY_VALUE_5, KEY_VALUE_9);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the '4' or '5' or '9' button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_4;
		key_value_list[1] = KEY_VALUE_5;
		key_value_list[2] = KEY_VALUE_9;
		key_value_cnt = 3;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the '4' or '5' or '9' key\n", PRINT_TAG);
			return ZAPPER_ERROR;
		}
		valid_cnt++;
	}

	/* key-4 */
	{
		/* wait press '6' or '8' or '0' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x 0x%x 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_6, KEY_VALUE_8, KEY_VALUE_0);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the '6' or '8' or '0' button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_6;
		key_value_list[1] = KEY_VALUE_8;
		key_value_list[2] = KEY_VALUE_0;
		key_value_cnt = 3;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the '6' or '8' or '0' key\n", PRINT_TAG);
			return ZAPPER_ERROR;
		}
		valid_cnt++;
	}

	/* key-5 */
	/* if the fourth key is not equal to KEY_VALUE_0, then need wait next key */
	if ((valid_cnt == 4) && (key_press_list[3] != KEY_VALUE_0)) {
		/* wait press '5' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_5);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the '5' button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_5;
		key_value_cnt = 1;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the '5' key\n", PRINT_TAG);
		} else {
			valid_cnt++;
		}
	}

	/* key-6 */
	/* if the fifth key is equal to KEY_VALUE_5, then need wait next key */
	if ((valid_cnt == 5) && (key_press_list[4] == KEY_VALUE_5)) {
		/* wait press 'i' key */
		memset(cmd_str, 0, sizeof(cmd_str));
		snprintf(cmd_str, sizeof(cmd_str), "%s %d 0x%x", RCU_KEY_DETECT_CMD, RCU_KEY_DETECT_TIMEOUT, KEY_VALUE_INFO);
		printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
		run_command(cmd_str, 0);

		/* Check if the INFO button has been pressed, otherwise return due to timeout */
		key_value_list[0] = KEY_VALUE_INFO;
		key_value_cnt = 1;
		if (check_and_output_key_value(key_value_list, key_value_cnt, &key_press_list[valid_cnt]) != 0) {
			printf("%s keyvalue: Not pressing the 'INFO' key\n", PRINT_TAG);
		} else {
			valid_cnt++;
		}
	}

	printf("%s valid key cnt: %d\n", PRINT_TAG, valid_cnt);
	/* get and save the RCU combination Key status */
	get_and_save_the_combination_type(key_press_list, valid_cnt);

	return ZAPPER_SUCCESS;
}

int Zapper_get_rcu_combination_type(unsigned char *type)
{
	printf("%s Hello, now we are going to do_zapper_key_detect\n", PRINT_TAG);
	int ret = ZAPPER_ERROR;

	/* LDRS's LED Behavior step 5 */
	Zapper_led_set(LED_POWER_RED);
	Zapper_led_set(LED_REMOTE_RED);
	Zapper_led_set(LED_ALERT_YELLOW);
	Zapper_led_show();

	ret = rcu_combination_key_detect();
	if ((ret != ZAPPER_SUCCESS) || (g_rcu_combinatoion_type == RCU_COMBINATION_MAX)) {
		/* LDRS's LED Behavior step 6 */
		if (Zapper_get_nand_standby_flag() == 0) {
			Zapper_led_set(LED_POWER_GREEN);
		} else {
			Zapper_led_set(LED_POWER_RED);
		}
		Zapper_led_set(LED_REMOTE_OFF);
		Zapper_led_set(LED_ALERT_OFF);
		Zapper_led_show();
		return ZAPPER_ERROR;
	}
	printf("%s g_rcu_combinatoion_type = %d\n", PRINT_TAG, g_rcu_combinatoion_type);
	*type = g_rcu_combinatoion_type;
	return ZAPPER_SUCCESS;
}
