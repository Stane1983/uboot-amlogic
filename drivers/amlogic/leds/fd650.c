#include "fd650.h"
#include <asm-generic/gpio.h>
#include <common.h>
#include <errno.h>
#include <dm.h>

#define		PIN_DATA			0
#define		PIN_CLK				1
#define DEFAULT_UDELAY			3

static int fd650_sda_get(struct fd650_bus *bus)
{
	int value;

	dm_gpio_set_dir_flags(&bus->gpios[PIN_DATA], GPIOD_IS_IN);
	value = dm_gpio_get_value(&bus->gpios[PIN_DATA]);
	dm_gpio_set_dir_flags(&bus->gpios[PIN_DATA], GPIOD_IS_OUT);

	return value;
}

static void fd650_sda_set(struct fd650_bus *bus, int bit)
{
	dm_gpio_set_value(&(bus->gpios[PIN_DATA]), bit);
}

static void fd650_scl_set(struct fd650_bus *bus, int bit)
{
	dm_gpio_set_value(&(bus->gpios[PIN_CLK]), bit);
}

static int fd650_ofdata_to_platdata(struct udevice *dev)
{
	struct fd650_bus *bus = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "sda-gpios", 0,
				   &bus->gpios[PIN_DATA], 0);
	if (ret < 0)
		goto error;
	ret = gpio_request_by_name(dev, "scl-gpios", 0,
				   &bus->gpios[PIN_CLK], 0);
	if (ret < 0)
		goto error;
	ret = dm_gpio_set_dir_flags(&bus->gpios[PIN_DATA],GPIOD_IS_OUT);
	if (ret) {
		printf("fd650 gpio sda set dir failed\n");
		return ret;
	}
	ret = dm_gpio_set_dir_flags(&bus->gpios[PIN_CLK],GPIOD_IS_OUT);
	if (ret) {
		printf("fd650 gpio scl set dir failed\n");
		return ret;
	}
	dm_gpio_set_value(&bus->gpios[PIN_DATA], 1);
	dm_gpio_set_value(&bus->gpios[PIN_CLK], 1);
	bus->udelay = dev_read_u32_default(dev, "fd650-gpio,delay-us",
					   DEFAULT_UDELAY);

	bus->get_sda = fd650_sda_get;
	bus->set_sda = fd650_sda_set;
	bus->set_scl = fd650_scl_set;

	return 0;
error:
	pr_err("Can't get %s gpios! Error: %d\n", dev->name, ret);
	return ret;
}

void fd650_start(struct fd650_bus *bus)
{
	bus->set_sda(bus, 1);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_sda(bus, 0);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);
}

void fd650_stop(struct fd650_bus *bus)
{
	bus->set_sda(bus, 0);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_sda(bus, 1);
	udelay(bus->udelay);
}

void fd650_wrbyte(struct fd650_bus *bus, u_int8 dat )
{
	u_int8 i;

	for (i = 0; i != 8; i++) {
		if (dat & 0x80)
			bus->set_sda(bus, 1);
		else
			bus->set_sda(bus, 0);
		udelay(bus->udelay);
		bus->set_scl(bus,1);
		dat <<= 1;
		udelay(bus->udelay);
		bus->set_scl(bus, 0);
	}
	bus->set_sda(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);
}

u_int8  fd650_rdbyte(struct fd650_bus *bus )
{
	u_int8 dat,i;
	bus->set_sda(bus, 1);
	dat = 0;
	for (i = 0; i != 8; i++) {
		udelay(bus->udelay);
		bus->set_scl(bus, 1);
		udelay(bus->udelay);
		dat <<= 1;
		if (bus->get_sda(bus))
			dat++;
		bus->set_scl(bus, 0);
	}
	bus->set_sda(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 1);
	udelay(bus->udelay);
	bus->set_scl(bus, 0);

	return dat;
}

void fd650_write(struct fd650_bus *bus, u_int16 cmd )
{
	fd650_start(bus);
	fd650_wrbyte(bus, ((u_int8)(cmd >> 7) & 0x3E) | 0x40);
	fd650_wrbyte(bus, (u_int8)cmd);
	fd650_stop(bus);

	return;
}

u_int8 fd650_read(struct fd650_bus *bus)
{
	u_int8 keycode = 0;

	fd650_start(bus);
	fd650_wrbyte(bus, ((u_int8)(FD650_GET_KEY >> 7) & 0x3E) | (0x01 | 0x40));
	keycode = fd650_rdbyte(bus);
	fd650_stop(bus);
	if ((keycode & 0x00000040) == 0)
		keycode = 0;
	return keycode;
}

static const struct udevice_id fd650_ids[] = {
	{ .compatible = "fd650" },
	{ }
};

U_BOOT_DRIVER(fd650) = {
	.name	= "fd650",
	.id	= UCLASS_LED,
	.of_match = fd650_ids,
	.of_to_plat = fd650_ofdata_to_platdata,
	.priv_auto	= sizeof(struct fd650_bus),
};
