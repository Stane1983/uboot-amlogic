#include <usb.h>
#include <amlogic/zapper_boot.h>
#include <command.h>
#include <string.h>
#include <stdbool.h>


static char zapper_usb_names[MAX_NAMES][NAME_LENGTH];	//Store a list of files in usb
static bool sdl_download = false;			//if sdl_download = true ,the usb zapper update is required

int Zapper_get_usb_download_request(void) {
	if (sdl_download) {
		return ZAPPER_FOUND_SDL;	// receive usb_download_request
	} else {
		return ZAPPER_NON_SDL;		// no usb_download_request
	}
}


int Zapper_read_usb_file_name(char *file_name, int num) {
	if (file_name == NULL || num < 0 || num >= MAX_NAMES) {
		printf("Input error\n");
		return ZAPPER_ERROR;
	}

	strncpy(zapper_usb_names[num], file_name, NAME_LENGTH - 1);
	zapper_usb_names[num][NAME_LENGTH - 1] = '\0';

	return ZAPPER_SUCCESS;

}


static int check_file_name(const char arr[MAX_NAMES][NAME_LENGTH], int size) {
	for (int i = 0; i < size; i++) {
		printf("arr[i]=%s\n",arr[i]);
		if (strstr(arr[i], "CD5") != NULL) {
			return ZAPPER_FOUND_SDL;  // sdl file exit
		} else if (strstr(arr[i], "VD5") != NULL) {
			return ZAPPER_FOUND_SDL;
		} else if (strstr(arr[i], "KD5") != NULL) {
			return ZAPPER_FOUND_SDL;
		}
	}
	return ZAPPER_NON_SDL;  // no sdl file exit
}

static int do_zapper_usb(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do usb detection\n");

	if (run_command(CMD_USB_START, NO_DETAIL) != 0) {
		printf("Command %s failed\n", CMD_USB_START);
		return ZAPPER_ERROR;
	}

	printf("Hello, now we are showing the contents of the USB flash drive in the USB port\n");

	if (run_command(CMD_USB_LS, NO_DETAIL) != 0) {
		printf("Command %s failed\n", CMD_USB_LS);
		return ZAPPER_ERROR;
	}

	sdl_download = check_file_name(zapper_usb_names, MAX_NAMES) ? ZAPPER_FOUND_SDL : ZAPPER_NON_SDL;

	return ZAPPER_SUCCESS;
}


U_BOOT_CMD(
	zapper_usb_detect, 1, 0, do_zapper_usb,"zapper usb detection" ,"It will use our usb driver and fat file system driver"
);

