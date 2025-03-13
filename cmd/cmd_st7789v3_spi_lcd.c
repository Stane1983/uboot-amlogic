#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>

#if CONFIG_IS_ENABLED(AMLOGIC_MODIFY)
#include <linux/compat.h>
#endif

int st7789v3_spi_init(void)
{
#if CONFIG_IS_ENABLED(DM_SPI)
	int ret = 0;
	char name[30], *str;
	// struct udevice *dev;
	// unsigned int	spi_mode = 0;
	// unsigned int	spi_freq = 20000000; //10Mhz
	struct spi_slave *slave;
	unsigned int	bus = 1;
	unsigned int	cs = 0;

	struct udevice *bus1;

	printf("st7789v3_spi_init\n");
	snprintf(name, sizeof(name), "st7789v3_spi_%d:%d", bus, cs);
	str = strdup(name);
	if (!str)
		return -1;
	// ret = _spi_get_bus_and_cs(bus, cs, spi_freq, spi_mode, "st7789v3",
				//   str, &dev, &slave);

	ret = spi_get_bus_and_cs(bus, cs, &bus1, &slave);
	if (ret)
		return ret;

	return ret;
#endif
}

int do_st7789v3_test(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	st7789v3_spi_init();
	return 0;
}

U_BOOT_CMD(
	st7789v3_test,	5,	1,	do_st7789v3_test,
	"st7789v3_test spi lcd test command",
	"[<bus>:]<cs>[.<mode>][@<freq>] <bit_len> <dout> - Send and receive bits\n"
	"<bus>     - Identifies the SPI bus\n"
	"<cs>      - Identifies the chip select\n"
	"<mode>    - Identifies the SPI mode to use\n"
	"<freq>    - Identifies the SPI bus frequency in Hz\n"
	"<bit_len> - Number of bits to send (base 10)\n"
	"<dout>    - Hexadecimal string that gets sent"
);
