#include <asm/io.h>
#include <common.h>
#include <command.h>
#include <vsprintf.h>
#include <amlogic/zapper_boot.h>
//#include <fcntl.h>
//#include <unistd.h>

#define LED_DEV_PATH "/sys/class/leds/fd650/fd650_display"

#define PRJ_TAG "ZAPPER"
#define RCU_TAG "<LED_DISPLAY>"
#define PRINT_TAG PRJ_TAG":"RCU_TAG

#define LED_DISPLAY_RED		'0'
#define LED_DISPLAY_GREEN	'1'
#define LED_DISPLAY_YELLOW	'2'
#define LED_DISPLAY_OFF		'l'

static char g_standby_led_show = LED_DISPLAY_OFF;
static char g_remote_led_show = LED_DISPLAY_OFF;
static char g_alert_led_show = LED_DISPLAY_OFF;

/* because the character 'l' will not display, so use it to mean off */
void Zapper_led_set(led_display_type type)
{
	switch (type) {
		case LED_POWER_RED:
			g_standby_led_show = LED_DISPLAY_RED;
			break;
		case LED_POWER_GREEN:
			g_standby_led_show = LED_DISPLAY_GREEN;
			break;
		case LED_POWER_OFF:
			g_standby_led_show = LED_DISPLAY_OFF;
			break;
		case LED_REMOTE_RED:
			g_remote_led_show = LED_DISPLAY_RED;
			break;
		case LED_REMOTE_OFF:
			g_remote_led_show = LED_DISPLAY_OFF;
			break;
		case LED_ALERT_YELLOW:
			g_alert_led_show = LED_DISPLAY_YELLOW;
			break;
		case LED_ALERT_OFF:
			g_alert_led_show = LED_DISPLAY_OFF;
			break;
		default:
			printf("%s invalid led type\n", PRINT_TAG);
			return;
	}

	printf("%s led type setting successful\n", PRINT_TAG);

	return;
}

void Zapper_led_show(void)
{
#ifdef CONFIG_BG20AB_S805C1
	printf("%s g_standby_led_show: %d, g_remote_led_show: %d, g_alert_led_show: %d\n",
		PRINT_TAG, g_standby_led_show, g_remote_led_show, g_alert_led_show);
	if (g_standby_led_show == LED_DISPLAY_RED) {
		run_command("gpio set gpiod_8", 0);
		run_command("gpio clear gpiod_9", 0);
	} else if (g_standby_led_show == LED_DISPLAY_GREEN) {
		run_command("gpio set gpiod_9", 0);
		run_command("gpio clear gpiod_8", 0);
	} else if (g_standby_led_show == LED_DISPLAY_OFF) {
		run_command("gpio clear gpiod_8", 0);
		run_command("gpio clear gpiod_9", 0);
	}

	if (g_remote_led_show == LED_DISPLAY_RED) {
		run_command("gpio set gpiod_7", 0);
	} else if (g_remote_led_show == LED_DISPLAY_OFF) {
		run_command("gpio clear gpiod_7", 0);
	}

	if (g_alert_led_show == LED_DISPLAY_YELLOW) {
		run_command("gpio set gpioz_7", 0);
	} else if (g_alert_led_show == LED_DISPLAY_OFF) {
		run_command("gpio clear gpioz_7", 0);
	}
#else
	char cmd_str[30] = { '\0' };

	sprintf(cmd_str, "led_display %c%c%c%c", g_standby_led_show, g_remote_led_show, g_alert_led_show, LED_DISPLAY_OFF);
	run_command(cmd_str, NO_DETAIL);
	printf("%s cmd_str: %s\n", PRINT_TAG, cmd_str);
#endif
	udelay(1000*50);
}

/* LDRS's LED Behavior step 1 */
static int do_zapper_board_init_led_set(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("%s: %d\n", __FUNCTION__, __LINE__);
	Zapper_led_set(LED_POWER_RED);
	Zapper_led_set(LED_REMOTE_OFF);
	Zapper_led_set(LED_ALERT_OFF);
	Zapper_led_show();

	return ZAPPER_SUCCESS;
}

/* LDRS's LED Behavior step 2 */
static int do_zapper_led_init_led_set(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("%s: %d\n", __FUNCTION__, __LINE__);
	Zapper_led_set(LED_POWER_GREEN);
	Zapper_led_set(LED_REMOTE_OFF);
	Zapper_led_set(LED_ALERT_OFF);
	Zapper_led_show();

	return ZAPPER_SUCCESS;
}

/* LDRS's LED Behavior step 3 */
static int do_zapper_board_late_init_led_set(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("%s: %d\n", __FUNCTION__, __LINE__);
	Zapper_led_set(LED_POWER_RED);
	Zapper_led_set(LED_REMOTE_OFF);
	Zapper_led_set(LED_ALERT_OFF);
	Zapper_led_show();

	return ZAPPER_SUCCESS;
}

/* LDRS's LED Behavior step 4 */
static int do_zapper_bmp_display_led_set(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	/* get standby_flag from loader partition */
	run_command("zapper_flash_read", 0);

	printf("%s: %d, standby_flag: %d\n", __FUNCTION__, __LINE__, Zapper_get_nand_standby_flag());
	if (Zapper_get_nand_standby_flag() == 0) {
		Zapper_led_set(LED_POWER_GREEN);
	} else {
		Zapper_led_set(LED_POWER_RED);
	}

	Zapper_led_set(LED_REMOTE_OFF);
	Zapper_led_set(LED_ALERT_OFF);

	Zapper_led_show();

	return ZAPPER_SUCCESS;
}

U_BOOT_CMD(
	zapper_board_init_led_set, 1, 0, do_zapper_board_init_led_set,"zapper led" ,"zapper led set for board init"
);
U_BOOT_CMD(
	zapper_led_init_led_set, 1, 0, do_zapper_led_init_led_set,"zapper led" ,"zapper led set for led init"
);
U_BOOT_CMD(
	zapper_board_late_init_led_set, 1, 0, do_zapper_board_late_init_led_set,"zapper led" ,"zapper led set for board late init"
);
U_BOOT_CMD(
	zapper_bmp_display_led_set, 1, 0, do_zapper_bmp_display_led_set,"zapper led" ,"zapper led set for board init"
);



