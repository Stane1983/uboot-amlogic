#include <amlogic/zapper_boot.h>
#include <command.h>
#include <stdio.h>


static unsigned char jump_recover_status = 0;// 0 no need to jump to recovery, > 0 need to jump to recovery 1: usb_detect 2: OTA detect 3: boot_check 4.UNKNOWN

int Zapper_get_jump_recovery_status(unsigned char* status)
{
	if (status == NULL) {
		return ZAPPER_ERROR;
	}

	*status = jump_recover_status;
	return ZAPPER_SUCCESS;

}

int Zapper_set_jump_recovery_status(unsigned char status)
{
	if (status < NO_NEED_JUMP || status > UNKNOWN_JUMP) {
		return ZAPPER_ERROR;
	}
	printf("[ZAPPER] set  jump_recover_status = %d\n",jump_recover_status);
	jump_recover_status = status;

	return ZAPPER_SUCCESS;
}

int Zapper_clear_jump_recovery_status(void)
{
	if (Zapper_set_jump_recovery_status(NO_NEED_JUMP))
		return ZAPPER_ERROR;
	else
		return ZAPPER_SUCCESS;
}



static int do_zapper_jump_recovery(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("%s:%d\n", __FUNCTION__, __LINE__);
	if (jump_recover_status) {
		run_command("reboot recovery", 0);
	}
	/* LDRS's LED Behavior step 7 */
	Zapper_led_set(LED_POWER_OFF);
	Zapper_led_set(LED_REMOTE_OFF);
	Zapper_led_set(LED_ALERT_OFF);
	Zapper_led_show();
	return ZAPPER_SUCCESS;
}


U_BOOT_CMD(
	zapper_jump_recovery, 1, 0, do_zapper_jump_recovery,"do_zapper_test_write" ,"It will use our yaffs2 driver"
);

