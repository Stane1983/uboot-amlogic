#include	"../drivers/amlogic/leds/fd650.h"
#include  <string.h>
#include <common.h>
#include <command.h>
#include <dm.h>
#include <led.h>
#include <dm/uclass-internal.h>

#define LEDMAPNUM 22

typedef struct _led_bitmap {
	u_int8 character;
	u_int8 bitmap;
} led_bitmap;

const led_bitmap bcd_decode_tab[LEDMAPNUM] = {
	{'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F},
	{'4', 0x66}, {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07},
	{'8', 0x7F}, {'9', 0x6F}, {'a', 0x77}, {'A', 0x77},
	{'b', 0x7C}, {'B', 0x7C}, {'c', 0x58}, {'C', 0x39},
	{'d', 0x5E}, {'D', 0x5E}, {'e', 0x79}, {'E', 0x79},
	{'f', 0x71}, {'F', 0x71}
};

u_int8 led_get_code(char ch)
{
	u_int8 i, bitmap = 0x00;

	for (i = 0; i < LEDMAPNUM; i++)
	{
		if (bcd_decode_tab[i].character == ch) {
			bitmap = bcd_decode_tab[i].bitmap;
			break;
		}
	}

	return bitmap;
}

void led_show_650( char *str, unsigned char sec_flag, unsigned char lock)
{
	int i, iLenth;
	int ret;
	int	data[4] = {0x00, 0x00, 0x00, 0x00};
	struct udevice *dev;
	struct fd650_bus *bus;

	ret = led_get_by_label("fd650", &dev);
	if (ret) {
		printf("LED 'fd650' not found (err=%d)\n", ret);
		return;
	}
	bus = dev_get_priv(dev);
	if (strcmp(str, "") == 0 )
		return;
	iLenth = strlen(str);
	if (iLenth > 4)
		iLenth = 4;
	for (i = 0; i < iLenth; i++) {
		data[i] = led_get_code(str[i]);
	}
	fd650_write(bus, FD650_SYSON_8);
	fd650_write(bus, FD650_DIG0 | (u_int8)data[0] | FD650_DOT);
	if (sec_flag)
		fd650_write(bus, FD650_DIG1 | (u_int8)data[1] | FD650_DOT );
	else
		fd650_write(bus, FD650_DIG1 | (u_int8)data[1] );
	if (lock)
		fd650_write(bus, FD650_DIG2 | (u_int8)data[2] | FD650_DOT);
	else
		fd650_write(bus, FD650_DIG2 | (u_int8)data[2]);
	fd650_write(bus, FD650_DIG3 | (u_int8)data[3] | FD650_DOT);
}

int do_led_display(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	char *display_str;

	/* Validate arguments */
	if (argc < 2)
		return CMD_RET_USAGE;
	display_str = argv[1];
	led_show_650(display_str, 1, 1);

	return 0;
}

U_BOOT_CMD(
	led_display, 4, 1, do_led_display,
	"display string on led",
	"led <string>\tShow string on led\n"
);
